// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/cpu.h>

#include <filesystem>
#include <format>
#include <map>
#include <string>
#include <vector>

#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

namespace {

const std::filesystem::path sysfs_cpu = "/sys/devices/system/cpu";

// sysfs cache sizes look like "32K", "1024K", "16M"
std::optional<Bytes> parse_cache_size(std::string_view s) {
  std::uint64_t factor = 1;
  switch (s.back()) {
    case 'K': factor = std::to_underlying(ByteUnit::KiB); break;
    case 'M': factor = std::to_underlying(ByteUnit::MiB); break;
    case 'G': factor = std::to_underlying(ByteUnit::GiB); break;
    default: s.remove_suffix(1);
  }
  const auto value = internal::parse<std::uint64_t>(s);
  return value ? std::optional<Bytes>({*value * factor}) : std::nullopt;
}

Cache read_cache(const std::filesystem::path& cpu_path) {
  Cache cache;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(cpu_path / "cache", ec)) {
    if (!entry.path().filename().string().starts_with("index")) {
      continue;
    }
    const auto level = internal::read_number_attribute<int>(entry.path() / "level");
    const auto type = internal::read_attribute(entry.path() / "type");
    const auto size = internal::read_attribute(entry.path() / "size").and_then(parse_cache_size);
    if (!level || !type) {
      continue;
    }
    if (*level == 1 && *type == "Data") {
      cache.l1_data = size;
    } else if (*level == 1 && *type == "Instruction") {
      cache.l1_instruction = size;
    } else if (*level == 2) {
      cache.l2 = size;
    } else if (*level == 3) {
      cache.l3 = size;
    }
  }
  return cache;
}

std::optional<Hertz> read_khz(const std::filesystem::path& path) {
  return internal::read_number_attribute<std::uint64_t>(path).transform(
      [](const std::uint64_t khz) { return khz * FrequencyUnit::kHz; });
}

}  // namespace

result<std::vector<Cpu>> cpus() {
  const auto processors = internal::read_file("/proc/cpuinfo").and_then(internal::procfs::parse_cpuinfo);
  if (!processors) {
    return std::unexpected(processors.error());
  }

  // socket id -> core id -> logical processors
  std::map<std::uint32_t, std::map<std::uint32_t, std::vector<const internal::procfs::CpuinfoProcessor*>>> sockets;
  for (const auto& p : *processors) {
    sockets[p.physical_id][p.core_id].push_back(&p);
  }

  std::vector<Cpu> result;
  result.reserve(sockets.size());
  for (const auto& [socket_id, cores] : sockets) {
    const auto& first = *cores.begin()->second.front();
    Cpu cpu{.socket = socket_id, .vendor = first.vendor, .model = first.model, .flags = first.flags};
    for (const auto& [core_id, threads] : cores) {
      const auto path = sysfs_cpu / std::format("cpu{}", threads.front()->processor);
      cpu.cores.push_back(Core{
          .id = core_id,
          .threads = static_cast<std::uint32_t>(threads.size()),
          .cache = read_cache(path),
          .base_frequency = read_khz(path / "cpufreq/base_frequency"),
          .max_frequency = read_khz(path / "cpufreq/cpuinfo_max_freq"),
      });
      cpu.logical_cores += static_cast<std::uint32_t>(threads.size());
    }
    cpu.physical_cores = static_cast<std::uint32_t>(cpu.cores.size());
    result.push_back(std::move(cpu));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
