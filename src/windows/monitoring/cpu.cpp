// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <winternl.h>
// clang-format on
#include <hwinfo/monitoring.h>

#include <cstdint>
#include <format>
#include <ranges>
#include <vector>

#include "internal/cpu_ticks.h"
#include "internal/windows_power.h"

#ifdef _MSC_VER
#pragma comment(lib, "ntdll.lib")
#endif

namespace hwinfo {

result<std::vector<detail::CpuTicks>> internal::read_cpu_ticks() {
  SYSTEM_INFO info{};
  GetSystemInfo(&info);
  std::vector<SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION> performance(info.dwNumberOfProcessors);
  ULONG length = 0;
  const NTSTATUS status =
      NtQuerySystemInformation(SystemProcessorPerformanceInformation, performance.data(),
                               static_cast<ULONG>(performance.size() * sizeof(performance.front())), &length);
  if (status != 0) {
    return std::unexpected(error{errc::platform_error, std::format("NtQuerySystemInformation: NTSTATUS {:#010x}",
                                                                   static_cast<unsigned long>(status))});
  }
  performance.resize(length / sizeof(performance.front()));

  // KernelTime includes IdleTime
  std::vector<detail::CpuTicks> ticks(1);  // [0]: aggregate over all cores
  ticks.reserve(performance.size() + 1);
  for (const auto& p : performance) {
    const auto total = static_cast<std::uint64_t>(p.KernelTime.QuadPart + p.UserTime.QuadPart);
    const auto idle = static_cast<std::uint64_t>(p.IdleTime.QuadPart);
    const detail::CpuTicks core{.busy = total >= idle ? total - idle : 0, .total = total};
    ticks.front().busy += core.busy;
    ticks.front().total += core.total;
    ticks.push_back(core);
  }
  return ticks;
}

result<std::vector<Hertz>> cpu_frequencies() {
  return internal::processor_power_information().transform([](const auto& processors) {
    return std::ranges::to<std::vector>(processors |
                                        std::views::transform([](const internal::ProcessorPowerInformation& p) {
                                          return std::uint64_t{p.current_mhz} * FrequencyUnit::MHz;
                                        }));
  });
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
