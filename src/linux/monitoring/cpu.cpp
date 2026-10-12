// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/monitoring.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <ranges>
#include <set>
#include <string>
#include <vector>

#include "internal/cpu_ticks.h"
#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

namespace {

const std::filesystem::path sysfs_cpu = "/sys/devices/system/cpu";

// Number of "possible" CPUs (online or not), e.g. 16 for "0-15".
result<std::uint32_t> possible_cpus() {
  const auto possible = internal::read_line(sysfs_cpu / "possible");
  if (!possible) {
    return std::unexpected(possible.error());
  }
  return internal::parse<std::uint32_t>(possible->substr(possible->find_last_of("-,") + 1)).transform([](auto last) {
    return last + 1;
  });
}

}  // namespace

result<std::vector<detail::CpuTicks>> internal::read_cpu_ticks() {
  auto ticks = read_file("/proc/stat").and_then(procfs::parse_stat);
  if (const auto possible = possible_cpus(); ticks && possible && ticks->size() < *possible + 1) {
    ticks->resize(*possible + 1);
  }
  return ticks;
}

result<PowerProfile> power_profile() {
  PowerProfile profile{.profile = internal::read_attribute("/sys/firmware/acpi/platform_profile")};
  std::set<std::string, std::less<>> governors;
  std::set<std::string, std::less<>> preferences;
  std::error_code ec;
  for (const auto& policy : std::filesystem::directory_iterator(sysfs_cpu / "cpufreq", ec)) {
    if (!policy.path().filename().string().starts_with("policy")) {
      continue;
    }
    if (auto governor = internal::read_attribute(policy.path() / "scaling_governor")) {
      governors.insert(std::move(*governor));
    }
    if (auto preference = internal::read_attribute(policy.path() / "energy_performance_preference")) {
      preferences.insert(std::move(*preference));
    }
  }
  profile.governors.assign(governors.begin(), governors.end());
  profile.energy_preferences.assign(preferences.begin(), preferences.end());
  return profile;
}

result<std::vector<Hertz>> cpu_frequencies() {
  const auto possible = possible_cpus();
  if (!possible) {
    return std::unexpected(possible.error());
  }
  if (!std::filesystem::exists(sysfs_cpu / "cpu0/cpufreq")) {
    return std::unexpected(error{errc::not_supported, "cpufreq is not available"});
  }
  return std::ranges::to<std::vector>(std::views::iota(0u, *possible) | std::views::transform([](std::uint32_t i) {
                                        const auto khz = internal::read_number_attribute<std::uint64_t>(
                                            sysfs_cpu / std::format("cpu{}/cpufreq/scaling_cur_freq", i));
                                        return khz.value_or(0) * FrequencyUnit::kHz;
                                      }));
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
