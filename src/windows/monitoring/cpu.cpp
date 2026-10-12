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
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "internal/cpu_ticks.h"
#include "internal/strings.h"
#include "internal/windows_power.h"
#include "internal/windows_strings.h"

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

namespace {

// Name of the active power plan, e.g. "Balanced".
std::optional<std::string> power_plan() {
  GUID* scheme = nullptr;
  if (PowerGetActiveScheme(nullptr, &scheme) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  std::optional<std::string> name;
  DWORD size = 0;
  if (PowerReadFriendlyName(nullptr, scheme, nullptr, nullptr, nullptr, &size) == ERROR_SUCCESS && size > 0) {
    std::wstring buffer(size / sizeof(wchar_t), L'\0');
    if (PowerReadFriendlyName(nullptr, scheme, nullptr, nullptr, reinterpret_cast<UCHAR*>(buffer.data()), &size) ==
        ERROR_SUCCESS) {
      name = internal::non_empty(internal::to_utf8(buffer.c_str()));
    }
  }
  LocalFree(scheme);
  return name;
}

// The power mode of Windows 10 1709+ (the slider of the battery flyout / the "Power mode" setting): an overlay over
// the power plan, read with PowerGetEffectiveOverlayScheme (not declared by MinGW).
std::optional<std::string> power_mode() {
  using GetOverlay = DWORD(WINAPI*)(GUID*);
  const HMODULE powrprof = GetModuleHandleW(L"powrprof.dll");
  const auto get = powrprof ? reinterpret_cast<GetOverlay>(
                                  reinterpret_cast<void*>(GetProcAddress(powrprof, "PowerGetEffectiveOverlayScheme")))
                            : nullptr;
  GUID overlay{};
  if (!get || get(&overlay) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  constexpr GUID best_power_efficiency{0x961cc777, 0x2547, 0x4f9d, {0x81, 0x74, 0x7d, 0x86, 0x18, 0x1b, 0x8a, 0x7a}};
  constexpr GUID better_performance{0x3af9b8d9, 0x7c97, 0x431d, {0xad, 0x78, 0x34, 0xa8, 0xbf, 0xea, 0x43, 0x9f}};
  constexpr GUID best_performance{0xded574b5, 0x45a0, 0x4f42, {0x87, 0x37, 0x46, 0x34, 0x5c, 0x09, 0xc2, 0x38}};
  if (IsEqualGUID(overlay, GUID{})) {
    return "balanced";
  }
  if (IsEqualGUID(overlay, best_power_efficiency)) {
    return "best power efficiency";
  }
  if (IsEqualGUID(overlay, better_performance)) {
    return "better performance";
  }
  if (IsEqualGUID(overlay, best_performance)) {
    return "best performance";
  }
  return std::nullopt;
}

}  // namespace

result<PowerProfile> power_profile() { return PowerProfile{.profile = power_mode(), .power_plan = power_plan()}; }

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
