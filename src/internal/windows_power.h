// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Per logical processor clock rates via CallNtPowerInformation (powrprof). Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <powrprof.h>
// clang-format on
#include <hwinfo/error.h>

#include <format>
#include <vector>

#ifdef _MSC_VER
#pragma comment(lib, "powrprof.lib")
#endif

namespace hwinfo::internal {

// PROCESSOR_POWER_INFORMATION (declared in the DDK only).
struct ProcessorPowerInformation {
  ULONG number;
  ULONG max_mhz;
  ULONG current_mhz;
  ULONG mhz_limit;
  ULONG max_idle_state;
  ULONG current_idle_state;
};

// One entry per logical processor of the current processor group.
inline result<std::vector<ProcessorPowerInformation>> processor_power_information() {
  SYSTEM_INFO info{};
  GetSystemInfo(&info);
  std::vector<ProcessorPowerInformation> result(info.dwNumberOfProcessors);
  const auto status = CallNtPowerInformation(ProcessorInformation, nullptr, 0, result.data(),
                                             static_cast<ULONG>(result.size() * sizeof(ProcessorPowerInformation)));
  if (status != 0) {
    return std::unexpected(error{errc::platform_error, std::format("CallNtPowerInformation: NTSTATUS {:#010x}",
                                                                   static_cast<unsigned long>(status))});
  }
  return result;
}

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
