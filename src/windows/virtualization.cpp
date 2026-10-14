// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/virtualization.h>
#include <windows.h>

#include <optional>
#include <string>

#include "internal/smbios.h"
#include "internal/strings.h"
#include "internal/virtualization.h"
#include "internal/win_registry.h"
#include "internal/windows_strings.h"

namespace hwinfo {

namespace {

std::optional<std::string> environment_variable(const wchar_t* name) {
  const DWORD size = GetEnvironmentVariableW(name, nullptr, 0);
  if (size == 0) {
    return std::nullopt;
  }
  std::wstring value(size, L'\0');
  value.resize(GetEnvironmentVariableW(name, value.data(), size));
  return internal::to_utf8(value);
}

std::optional<VirtualMachine> virtual_machine() {
  std::optional<internal::CpuidHypervisor> cpuid;
#ifdef HWINFO_X86
  cpuid = internal::cpuid_hypervisor();
#endif
  const std::wstring bios = L"HARDWARE\\DESCRIPTION\\System\\BIOS";
  const auto vendor = internal::registry::read_string(HKEY_LOCAL_MACHINE, bios, L"SystemManufacturer");
  const auto product = internal::registry::read_string(HKEY_LOCAL_MACHINE, bios, L"SystemProductName");
  const auto table = smbios::read_system_table();
  return internal::classify_vm(cpuid, internal::hypervisor_from_dmi(vendor.value_or(""), product.value_or("")),
                               table && internal::smbios_vm_flag(*table));
}

// Windows containers (process isolation). Hyper-V isolated containers run in a utility VM and are detected as Hyper-V
// VM by virtual_machine(). The runtime (Docker, containerd) is not exposed.
std::optional<Container> container() {
  const bool kubernetes = environment_variable(L"KUBERNETES_SERVICE_HOST").has_value();
  const auto user = environment_variable(L"USERNAME").value_or("");
  if (internal::registry::read_dword(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Control", L"ContainerType") ||
      internal::equals_ignore_case(user, "ContainerAdministrator") ||
      internal::equals_ignore_case(user, "ContainerUser") || kubernetes) {
    return Container{.runtime = ContainerRuntime::unknown, .kubernetes = kubernetes};
  }
  return std::nullopt;
}

}  // namespace

result<Virtualization> virtualization() { return Virtualization{.vm = virtual_machine(), .container = container()}; }

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
