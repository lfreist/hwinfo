// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/cpu.h>
#include <mach/machine.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/strings.h"
#include "internal/sysctl.h"

namespace hwinfo {

namespace {

bool is_apple_silicon() {
  return internal::sysctl_string("hw.machine").value_or(std::string{}).contains("arm64") ||
         internal::sysctl_number<int>("hw.optional.arm64") == 1;
}

bool is_powerpc() { return internal::sysctl_number<int>("hw.cputype") == CPU_TYPE_POWERPC; }

std::string powerpc_model() {
  const int subtype = internal::sysctl_number<int>("hw.cpusubtype").value_or(-1);
  switch (subtype) {
    case 100:
      return "PowerPC G5 (970)";
    case 11:
      return "PowerPC G4 (7450)";
    case 10:
      return "PowerPC G4 (7400)";
    case 9:
      return "PowerPC G3 (750)";
    case 1:
      return "PowerPC 601";
    default:
      return std::format("PowerPC (unknown subtype: {})", subtype);
  }
}

std::string vendor() {
  if (auto vendor = internal::sysctl_attribute("machdep.cpu.vendor")) {
    return std::move(*vendor);  // Intel: "GenuineIntel"
  }
  if (is_apple_silicon()) {
    return "Apple";
  }
  if (is_powerpc()) {
    return "IBM";
  }
  return {};
}

std::string model() {
  if (auto brand = internal::sysctl_attribute("machdep.cpu.brand_string")) {
    return std::move(*brand);  // e.g. "Apple M1 Pro", "Intel(R) Core(TM) i7-9750H CPU @ 2.60GHz"
  }
  if (is_apple_silicon()) {
    return "Apple Silicon";
  }
  if (is_powerpc()) {
    return powerpc_model();
  }
  return {};
}

void add_flag(std::vector<std::string>& flags, std::string flag) {
  if (std::ranges::find(flags, flag) == flags.end()) {
    flags.push_back(std::move(flag));
  }
}

std::string to_lower(std::string_view s) {
  std::string out(s);
  std::ranges::transform(out, out.begin(),
                         [](char c) { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; });
  return out;
}

std::vector<std::string> x86_flags() {
  std::vector<std::string> flags;
  for (const char* name : {"machdep.cpu.features", "machdep.cpu.extfeatures", "machdep.cpu.leaf7_features"}) {
    const auto features = internal::sysctl_string(name);
    if (!features) {
      continue;
    }
    for (const auto word : internal::words(*features)) {
      std::string flag = to_lower(word);
      if (flag == "avx1.0") {
        flag = "avx";
      }
      std::ranges::replace(flag, '.', '_');  // "sse4.1" -> "sse4_1"
      add_flag(flags, std::move(flag));
    }
  }
  return flags;
}

std::vector<std::string> arm_flags() {
  constexpr std::string_view features[]{
      "AES",         "PMULL",      "SHA1",       "SHA256", "SHA512", "SHA3",  "CRC32",  "LSE",    "LSE2",
      "FP16",        "FHM",        "DotProd",    "RDM",    "JSCVT",  "FCMA",  "LRCPC",  "LRCPC2", "FRINTTS",
      "BF16",        "EBF16",      "I8MM",       "SME",    "SME2",   "SB",    "SSBS",   "BTI",    "DPB",
      "DPB2",        "SME_F64F64", "SME_I16I64", "FlagM",  "FlagM2", "PAuth", "PAuth2", "FPAC",   "ECV",
      "FPACCOMBINE", "AFP",        "RPRES",      "DIT",    "CSV2",   "CSV3",  "CSSC",   "WFxT",
  };
  std::vector<std::string> flags;
  if (internal::sysctl_number<int>("hw.optional.AdvSIMD") == 1 ||
      internal::sysctl_number<int>("hw.optional.neon") == 1) {
    add_flag(flags, "neon");
  }
  if (internal::sysctl_number<int>("hw.optional.floatingpoint") == 1) {
    add_flag(flags, "fp");
  }
  if (internal::sysctl_number<int>("hw.optional.armv8_crc32") == 1) {  // macOS 11
    add_flag(flags, "crc32");
  }
  for (const auto feature : features) {
    const auto name = std::format("hw.optional.arm.FEAT_{}", feature);
    if (internal::sysctl_number<int>(name.c_str()) == 1) {
      add_flag(flags, to_lower(feature));
    }
  }
  return flags;
}

std::vector<std::string> flags() {
  if (internal::sysctl_string("machdep.cpu.features")) {
    return x86_flags();
  }
  if (is_apple_silicon()) {
    return arm_flags();
  }
  return {};
}

std::optional<Bytes> cache_size(const std::string& name) {
  const auto size = internal::sysctl_number<std::uint64_t>(name.c_str());
  return size && *size > 0 ? std::optional<Bytes>(Bytes{*size}) : std::nullopt;
}

std::optional<Hertz> frequency(const char* name) {
  const auto hz = internal::sysctl_number<std::uint64_t>(name);
  return hz && *hz > 0 ? std::optional<Hertz>(Hertz{*hz}) : std::nullopt;
}

// Cores of the same type: the performance and efficiency clusters of Apple Silicon, or all cores of an Intel CPU.
struct CoreGroup {
  std::uint32_t physical = 0;
  std::uint32_t logical = 0;
  Cache cache{};
};

std::vector<CoreGroup> perf_levels(const Cache& fallback) {
  const auto count = internal::sysctl_number<std::uint32_t>("hw.nperflevels").value_or(0);
  std::vector<CoreGroup> groups;
  // Logical CPU numbers start with the efficiency cores: list the slowest level first.
  for (std::uint32_t level = count; level-- > 0;) {
    const auto prefix = std::format("hw.perflevel{}.", level);
    const auto physical = internal::sysctl_number<std::uint32_t>((prefix + "physicalcpu").c_str());
    const auto logical = internal::sysctl_number<std::uint32_t>((prefix + "logicalcpu").c_str());
    if (!physical || !logical || *physical == 0) {
      return {};
    }
    const auto level_cache = [&](std::string_view key, const std::optional<Bytes>& otherwise) {
      auto size = cache_size(prefix + std::string(key));
      return size ? size : otherwise;
    };
    groups.push_back(CoreGroup{
        .physical = *physical,
        .logical = *logical,
        .cache =
            Cache{
                .l1_data = level_cache("l1dcachesize", fallback.l1_data),
                .l1_instruction = level_cache("l1icachesize", fallback.l1_instruction),
                .l2 = level_cache("l2cachesize", fallback.l2),
                .l3 = level_cache("l3cachesize", fallback.l3),
            },
    });
  }
  return groups;
}

}  // namespace

result<std::vector<Cpu>> cpus() {
  const auto physical = internal::sysctl_value<std::uint32_t>("hw.physicalcpu");
  if (!physical) {
    return std::unexpected(physical.error());
  }
  const auto logical = internal::sysctl_value<std::uint32_t>("hw.logicalcpu");
  if (!logical) {
    return std::unexpected(logical.error());
  }

  const auto base_frequency = frequency("hw.cpufrequency");
  const auto max_frequency = frequency("hw.cpufrequency_max");
  const Cache cache{
      .l1_data = cache_size("hw.l1dcachesize"),
      .l1_instruction = cache_size("hw.l1icachesize"),
      .l2 = cache_size("hw.l2cachesize"),
      .l3 = cache_size("hw.l3cachesize"),
  };

  auto groups = perf_levels(cache);
  if (groups.empty()) {
    groups.push_back(CoreGroup{.physical = *physical, .logical = *logical, .cache = cache});
  }

  Cpu cpu{
      .socket = 0,
      .vendor = vendor(),
      .model = model(),
      .physical_cores = *physical,
      .logical_cores = *logical,
      .flags = flags(),
      .cores = {},
  };
  for (const auto& group : groups) {
    const std::uint32_t threads = std::max(1u, group.logical / std::max(1u, group.physical));
    for (std::uint32_t i = 0; i < group.physical; ++i) {
      cpu.cores.push_back(Core{
          .id = static_cast<std::uint32_t>(cpu.cores.size()),
          .threads = threads,
          .cache = group.cache,
          .base_frequency = base_frequency,
          .max_frequency = max_frequency,
      });
    }
  }
  return std::vector<Cpu>{std::move(cpu)};
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
