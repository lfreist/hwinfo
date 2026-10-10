// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Parsers for Linux procfs / sysfs / etc files. Pure functions on the file content, so they can be unit tested on any
// platform. Not part of the public API.

#pragma once

#include <hwinfo/error.h>
#include <hwinfo/monitoring.h>
#include <hwinfo/units.h>

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "strings.h"

namespace hwinfo::internal::procfs {

// ----- /proc/cpuinfo ------------------------------------------------------------------------------------------------

// One "processor" block of /proc/cpuinfo, i.e. one logical core.
struct CpuinfoProcessor {
  std::uint32_t processor = 0;
  std::uint32_t physical_id = 0;
  std::uint32_t core_id = 0;
  std::string vendor;
  std::string model;
  std::vector<std::string> flags;
  std::optional<double> mhz;
};

// Vendor names for the ARM "CPU implementer" field.
constexpr std::string_view arm_implementer(std::uint32_t implementer) noexcept {
  switch (implementer) {
    case 0x41:
      return "ARM";
    case 0x42:
      return "Broadcom";
    case 0x43:
      return "Cavium";
    case 0x46:
      return "Fujitsu";
    case 0x48:
      return "HiSilicon";
    case 0x4e:
      return "NVIDIA";
    case 0x50:
      return "Applied Micro";
    case 0x51:
      return "Qualcomm";
    case 0x53:
      return "Samsung";
    case 0x56:
      return "Marvell";
    case 0x61:
      return "Apple";
    case 0x69:
      return "Intel";
    case 0xc0:
      return "Ampere";
    default:
      return {};
  }
}

inline result<std::vector<CpuinfoProcessor>> parse_cpuinfo(std::string_view content) {
  using Block = std::map<std::string_view, std::string_view, std::less<>>;
  const auto get = [](const Block& block, std::initializer_list<std::string_view> keys) -> std::string_view {
    for (const auto key : keys) {
      if (const auto it = block.find(key); it != block.end() && !it->second.empty()) {
        return it->second;
      }
    }
    return {};
  };

  std::vector<Block> processors;
  Block global;  // some architectures (ARM, RISC-V) list system wide fields outside of the processor blocks
  Block current;
  const auto finish_block = [&] {
    if (current.contains("processor")) {
      processors.push_back(std::move(current));
    } else {
      global.insert(current.begin(), current.end());
    }
    current.clear();
  };
  for (const auto line : lines(content)) {
    if (trim(line).empty()) {
      finish_block();
      continue;
    }
    if (const auto kv = split_key_value(line, ':')) {
      current.emplace(kv->first, kv->second);
    }
  }
  finish_block();

  if (processors.empty()) {
    return std::unexpected(error{errc::parse_error, "/proc/cpuinfo: no processor entries"});
  }

  const std::string_view global_model = get(global, {"model name", "Processor", "cpu model", "Model", "uarch"});
  const std::string_view global_vendor = get(global, {"vendor_id", "Hardware"});

  std::vector<CpuinfoProcessor> result;
  result.reserve(processors.size());
  for (const auto& block : processors) {
    CpuinfoProcessor p;
    auto processor = parse<std::uint32_t>(block.at("processor"));
    if (!processor) {
      return std::unexpected(processor.error());
    }
    p.processor = *processor;
    p.physical_id = parse<std::uint32_t>(get(block, {"physical id"})).value_or(0);
    p.core_id = parse<std::uint32_t>(get(block, {"core id"})).value_or(p.processor);

    p.vendor = get(block, {"vendor_id", "vendor"});
    if (p.vendor.empty()) {
      p.vendor = arm_implementer(parse<std::uint32_t>(get(block, {"CPU implementer"}), 16).value_or(0));
    }
    if (p.vendor.empty()) {
      p.vendor = global_vendor;
    }
    p.model = get(block, {"model name", "cpu model", "Processor", "uarch"});
    if (p.model.empty()) {
      p.model = global_model;
    }
    p.flags = std::ranges::to<std::vector<std::string>>(words(get(block, {"flags", "Features", "isa"})));
    if (const auto mhz = parse<double>(get(block, {"cpu MHz"}))) {
      p.mhz = *mhz;
    }
    result.push_back(std::move(p));
  }
  return result;
}

// ----- /sys/devices/system/cpu/cpuN/cache/indexM/size
// -----------------------------------------------------------------

// Sizes look like "48K", "1280K" or "16M" (binary units); a plain number is in bytes.
inline std::optional<Bytes> parse_cache_size(std::string_view s) {
  s = trim(s);
  if (s.empty()) {
    return std::nullopt;
  }
  ByteUnit unit = ByteUnit::B;
  switch (s.back()) {
    case 'K':
      unit = ByteUnit::KiB;
      break;
    case 'M':
      unit = ByteUnit::MiB;
      break;
    case 'G':
      unit = ByteUnit::GiB;
      break;
    case 'T':
      unit = ByteUnit::TiB;
      break;
    default:
      break;
  }
  if (unit != ByteUnit::B) {
    s.remove_suffix(1);
  }
  const auto value = parse<std::uint64_t>(s);
  return value ? std::optional(*value * unit) : std::nullopt;
}

// ----- /proc/stat ---------------------------------------------------------------------------------------------------

// Returns the tick counters of the "cpu" line at [0] and those of "cpuN" at [1 + N]. Offline CPUs have no line and
// keep zero ticks, so the index always matches the OS CPU number.
inline result<std::vector<detail::CpuTicks>> parse_stat(std::string_view content) {
  std::vector<detail::CpuTicks> ticks;
  bool has_aggregate = false;
  for (const auto line : lines(content)) {
    if (!line.starts_with("cpu")) {
      continue;
    }
    auto fields = words(line);
    const std::string_view name = *fields.begin();
    std::size_t index = 0;
    if (name != "cpu") {
      const auto number = parse<std::uint32_t>(name.substr(3));
      if (!number) {
        return std::unexpected(error{errc::parse_error, "/proc/stat"});
      }
      index = std::size_t{*number} + 1;
    }
    // cpu  user nice system idle iowait irq softirq steal guest guest_nice
    // guest times are already included in user / nice.
    std::uint64_t values[8]{};
    std::size_t n = 0;
    for (const auto word : fields | std::views::drop(1) | std::views::take(8)) {
      auto value = parse<std::uint64_t>(word);
      if (!value) {
        return std::unexpected(error{errc::parse_error, "/proc/stat"});
      }
      values[n++] = *value;
    }
    if (n < 4) {
      return std::unexpected(error{errc::parse_error, "/proc/stat"});
    }
    detail::CpuTicks t;
    for (const auto v : values) {
      t.total += v;
    }
    t.busy = t.total - values[3] - values[4];  // idle + iowait
    if (ticks.size() <= index) {
      ticks.resize(index + 1);
    }
    ticks[index] = t;
    has_aggregate |= index == 0;
  }
  if (!has_aggregate) {
    return std::unexpected(error{errc::parse_error, "/proc/stat: no cpu entries"});
  }
  return ticks;
}

// ----- /proc/meminfo ------------------------------------------------------------------------------------------------

inline result<MemoryUsage> parse_meminfo(std::string_view content) {
  std::optional<std::uint64_t> total, free, available;
  for (const auto line : lines(content)) {
    const auto kv = split_key_value(line, ':');
    if (!kv) {
      continue;
    }
    // values are given in KiB: "MemTotal:       32617072 kB"
    const auto kib = [&]() -> std::uint64_t {
      auto value = words(kv->second);
      return value.begin() == value.end() ? 0 : parse<std::uint64_t>(*value.begin()).value_or(0) * 1024;
    };
    if (kv->first == "MemTotal") {
      total = kib();
    } else if (kv->first == "MemFree") {
      free = kib();
    } else if (kv->first == "MemAvailable") {
      available = kib();
    }
  }
  if (!total || !free) {
    return std::unexpected(error{errc::parse_error, "/proc/meminfo"});
  }
  // MemAvailable is missing on kernels < 3.14
  return MemoryUsage{{*total}, {*free}, {available.value_or(*free)}};
}

// ----- /etc/os-release ----------------------------------------------------------------------------------------------

inline std::map<std::string, std::string, std::less<>> parse_os_release(std::string_view content) {
  std::map<std::string, std::string, std::less<>> values;
  for (const auto line : lines(content)) {
    if (line.starts_with('#')) {
      continue;
    }
    if (const auto kv = split_key_value(line, '=')) {
      values.emplace(kv->first, unquote(kv->second));
    }
  }
  return values;
}

// Release name of parsed os-release values, e.g. "Noble Numbat" (Ubuntu), "bookworm" (Debian), "Virginia" (Mint).
inline std::string os_release_codename(const std::map<std::string, std::string, std::less<>>& values) {
  auto codename = values.find("VERSION_CODENAME");
  if (codename == values.end() || codename->second.empty()) {
    codename = values.find("UBUNTU_CODENAME");  // older Ubuntu releases and derivatives
  }
  if (codename == values.end() || codename->second.empty()) {
    return {};
  }
  if (const auto version = values.find("VERSION"); version != values.end()) {
    const std::string_view v = version->second;
    const auto open = v.rfind('(');
    const auto close = v.rfind(')');
    if (open != std::string_view::npos && close != std::string_view::npos && open < close) {
      const auto display = trim(v.substr(open + 1, close - open - 1));
      if (display.size() >= codename->second.size() &&
          equals_ignore_case(display.substr(0, codename->second.size()), codename->second)) {
        return std::string(display);
      }
    }
  }
  return codename->second;
}

// ----- /proc/self/mounts --------------------------------------------------------------------------------------------

struct Mount {
  std::string device;
  std::string mount_point;
  std::string fs_type;
};

// Decodes the octal escapes (\040 for space, ...) used in /proc/self/mounts.
inline std::string unescape_mount_path(std::string_view s) {
  std::string out;
  out.reserve(s.size());
  for (std::size_t i = 0; i < s.size(); ++i) {
    if (s[i] == '\\' && i + 3 < s.size()) {
      if (auto code = parse<unsigned>(s.substr(i + 1, 3), 8)) {
        out.push_back(static_cast<char>(*code));
        i += 3;
        continue;
      }
    }
    out.push_back(s[i]);
  }
  return out;
}

inline std::vector<Mount> parse_mounts(std::string_view content) {
  std::vector<Mount> mounts;
  for (const auto line : lines(content)) {
    auto fields = words(line);
    auto it = fields.begin();
    if (it == fields.end()) {
      continue;
    }
    const std::string_view device = *it;
    if (++it == fields.end()) {
      continue;
    }
    const std::string_view mount_point = *it;
    const std::string_view fs_type = ++it == fields.end() ? std::string_view{} : *it;
    mounts.push_back({unescape_mount_path(device), unescape_mount_path(mount_point), std::string(fs_type)});
  }
  return mounts;
}

// ----- /proc/self/mountinfo -----------------------------------------------------------------------------------------

struct MountInfo {
  std::string root;
  std::string mount_point;
  std::string fs_type;
  std::string super_options;
};

// Format: "<id> <parent> <major:minor> <root> <mount point> <options> [optional fields...] - <type> <source> <super
// options>"
inline std::vector<MountInfo> parse_mountinfo(std::string_view content) {
  std::vector<MountInfo> mounts;
  for (const auto line : lines(content)) {
    const auto fields = std::ranges::to<std::vector<std::string_view>>(words(line));
    const auto separator = std::ranges::find(fields, "-");
    const auto index = static_cast<std::size_t>(separator - fields.begin());
    if (index < 6 || fields.size() < index + 2) {
      continue;
    }
    mounts.push_back({
        .root = unescape_mount_path(fields[3]),
        .mount_point = unescape_mount_path(fields[4]),
        .fs_type = std::string(fields[index + 1]),
        .super_options = fields.size() > index + 3 ? std::string(fields[index + 3]) : std::string{},
    });
  }
  return mounts;
}

// ----- cgroups ------------------------------------------------------------------------------------------------------

struct CgroupMembership {
  std::optional<std::string> unified{};
  std::map<std::string, std::string, std::less<>> controllers{};
};

// /proc/self/cgroup, e.g. "0::/user.slice" (v2) or "4:cpu,cpuacct:/docker/<id>" (v1)
inline CgroupMembership parse_cgroup(std::string_view content) {
  CgroupMembership membership;
  for (const auto line : lines(content)) {
    const auto first = line.find(':');
    const auto second = first == std::string_view::npos ? first : line.find(':', first + 1);
    if (second == std::string_view::npos) {
      continue;
    }
    const auto controllers = line.substr(first + 1, second - first - 1);
    const std::string path(line.substr(second + 1));
    if (controllers.empty()) {
      membership.unified = path;
    } else {
      for (const auto controller : split(controllers, ',')) {
        membership.controllers.emplace(controller, path);
      }
    }
  }
  return membership;
}

// cgroup v2 cpu.max: "<quota> <period>" in microseconds, quota "max" if unlimited. Returns the quota in cores.
inline std::optional<double> parse_cgroup_cpu_max(std::string_view content) {
  const auto fields = std::ranges::to<std::vector<std::string_view>>(words(content));
  if (fields.empty() || fields[0] == "max") {
    return std::nullopt;
  }
  const auto quota = parse<std::uint64_t>(fields[0]);
  const auto period = parse<std::uint64_t>(fields.size() > 1 ? fields[1] : std::string_view("100000"));
  if (!quota || !period || *period == 0) {
    return std::nullopt;
  }
  return static_cast<double>(*quota) / static_cast<double>(*period);
}

// cgroup v1 cpu.cfs_quota_us / cpu.cfs_period_us: quota -1 if unlimited.
inline std::optional<double> parse_cgroup_cfs_quota(std::string_view quota, std::string_view period) {
  const auto q = parse<std::int64_t>(trim(quota));
  const auto p = parse<std::int64_t>(trim(period));
  if (!q || !p || *q <= 0 || *p <= 0) {
    return std::nullopt;
  }
  return static_cast<double>(*q) / static_cast<double>(*p);
}

// cgroup v2 memory.max ("max" if unlimited) or v1 memory.limit_in_bytes (close to INT64_MAX if unlimited).
inline std::optional<Bytes> parse_cgroup_memory_limit(std::string_view content) {
  content = trim(content);
  if (content == "max") {
    return std::nullopt;
  }
  const auto value = parse<std::uint64_t>(content);
  if (!value || *value >= (std::uint64_t{1} << 62)) {
    return std::nullopt;
  }
  return Bytes{*value};
}

}  // namespace hwinfo::internal::procfs
