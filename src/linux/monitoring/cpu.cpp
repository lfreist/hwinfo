// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/monitoring.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <ranges>
#include <vector>

#include "internal/cpu_ticks.h"
#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

result<std::vector<detail::CpuTicks>> internal::read_cpu_ticks() {
  return read_file("/proc/stat").and_then(procfs::parse_stat);
}

result<std::vector<Hertz>> cpu_frequencies() {
  const std::filesystem::path sysfs_cpu = "/sys/devices/system/cpu";
  // "possible" CPUs, e.g. "0-15"
  auto possible = internal::read_line(sysfs_cpu / "possible");
  if (!possible) {
    return std::unexpected(possible.error());
  }
  const auto last = internal::parse<std::uint32_t>(possible->substr(possible->find_last_of("-,") + 1));
  if (!last) {
    return std::unexpected(last.error());
  }
  if (!std::filesystem::exists(sysfs_cpu / "cpu0/cpufreq")) {
    return std::unexpected(error{errc::not_supported, "cpufreq is not available"});
  }
  return std::views::iota(0u, *last + 1) | std::views::transform([&](std::uint32_t i) {
           const auto khz = internal::read_number_attribute<std::uint64_t>(
               sysfs_cpu / std::format("cpu{}/cpufreq/scaling_cur_freq", i));
           return khz.value_or(0) * FrequencyUnit::kHz;
         }) |
         std::ranges::to<std::vector>();
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
