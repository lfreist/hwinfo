// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/os.h>
#include <windows.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>

#include "internal/win_registry.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

const std::wstring current_version = L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion";

struct Architecture {
  std::string name;
  unsigned bits = 0;
};

Architecture architecture_of_machine(USHORT machine) {
  switch (machine) {
    case IMAGE_FILE_MACHINE_AMD64:
      return {"x86_64", 64};
    case IMAGE_FILE_MACHINE_ARM64:
      return {"aarch64", 64};
    case IMAGE_FILE_MACHINE_I386:
      return {"x86", 32};
    case IMAGE_FILE_MACHINE_ARMNT:
      return {"arm", 32};
    default:
      return {std::format("unknown ({:#06x})", machine), 0};
  }
}

// Native architecture of the system, also when running emulated (e.g. an x86_64 build on Windows on ARM).
Architecture native_architecture() {
  // IsWow64Process2 is available since Windows 10 1709: resolved at runtime to keep older systems supported.
  using IsWow64Process2Fn = BOOL(WINAPI*)(HANDLE, USHORT*, USHORT*);
  if (const HMODULE kernel32 = GetModuleHandleW(L"kernel32.dll")) {
    const auto proc = GetProcAddress(kernel32, "IsWow64Process2");
    USHORT process_machine = 0;
    USHORT native_machine = 0;
    if (proc != nullptr && reinterpret_cast<IsWow64Process2Fn>(reinterpret_cast<void*>(proc))(
                               GetCurrentProcess(), &process_machine, &native_machine)) {
      return architecture_of_machine(native_machine);
    }
  }
  SYSTEM_INFO info{};
  GetNativeSystemInfo(&info);
  switch (info.wProcessorArchitecture) {
    case PROCESSOR_ARCHITECTURE_AMD64:
      return {"x86_64", 64};
    case PROCESSOR_ARCHITECTURE_ARM64:
      return {"aarch64", 64};
    case PROCESSOR_ARCHITECTURE_INTEL:
      return {"x86", 32};
    case PROCESSOR_ARCHITECTURE_ARM:
      return {"arm", 32};
    default:
      return {std::format("unknown ({})", info.wProcessorArchitecture), 0};
  }
}

// "<build>.<update build revision>", e.g. "22631.4317"
std::optional<std::string> kernel_build(std::optional<std::string> build) {
  if (!build) {
    build = internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"CurrentBuildNumber");
  }
  const auto revision = internal::registry::read_dword(HKEY_LOCAL_MACHINE, current_version, L"UBR");
  if (build && revision) {
    return std::format("{}.{}", *build, *revision);
  }
  return build;
}

// e.g. "24H2"; Windows 10 releases before 20H2 only provide the numeric "ReleaseId", e.g. "1909"
std::string marketing_name() {
  if (auto name = internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"DisplayVersion")) {
    return std::move(*name);
  }
  return internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"ReleaseId").value_or("");
}

}  // namespace

result<Os> os() {
  const auto [arch, bits] = native_architecture();
  const auto rows = internal::wmi::query("Win32_OperatingSystem", {"Caption", "Version", "BuildNumber"});
  if (rows && !rows->empty()) {
    const auto& row = rows->front();
    return Os{
        .family = OsFamily::windows,
        .name = row.string("Caption").value_or("Microsoft Windows"),
        .marketing_name = marketing_name(),
        .version = row.string("Version").value_or(""),
        .kernel = kernel_build(row.string("BuildNumber")).value_or(""),
        .architecture = arch,
        .bits = bits,
    };
  }

  const auto name = internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"ProductName");
  if (!name) {
    return std::unexpected(rows ? error{errc::not_found, "WMI: Win32_OperatingSystem"} : rows.error());
  }
  const auto build = internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"CurrentBuildNumber");
  const auto major = internal::registry::read_dword(HKEY_LOCAL_MACHINE, current_version, L"CurrentMajorVersionNumber");
  const auto minor = internal::registry::read_dword(HKEY_LOCAL_MACHINE, current_version, L"CurrentMinorVersionNumber");
  std::string version;
  if (major && minor && build) {
    version = std::format("{}.{}.{}", *major, *minor, *build);
  } else if (const auto legacy =
                 internal::registry::read_string(HKEY_LOCAL_MACHINE, current_version, L"CurrentVersion");
             legacy && build) {
    version = std::format("{}.{}", *legacy, *build);  // Windows < 10, e.g. "6.3.9600"
  }
  return Os{
      .family = OsFamily::windows,
      .name = std::format("Microsoft {}", *name),
      .marketing_name = marketing_name(),
      .version = std::move(version),
      .kernel = kernel_build(build).value_or(""),
      .architecture = arch,
      .bits = bits,
  };
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
