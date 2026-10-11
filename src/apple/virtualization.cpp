// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/virtualization.h>

#include <optional>
#include <string>

#include "internal/sysctl.h"
#include "internal/virtualization.h"

namespace hwinfo {

namespace {

std::optional<VirtualMachine> virtual_machine() {
  std::optional<internal::CpuidHypervisor> cpuid;
#ifdef HWINFO_X86
  cpuid = internal::cpuid_hypervisor();
#endif
  const auto present = internal::sysctl_number<int>("kern.hv_vmm_present");
  if (present ? *present == 0 : !cpuid) {
    return std::nullopt;
  }
  std::optional<Hypervisor> model;
  if (internal::sysctl_attribute("hw.model").value_or("").starts_with("VirtualMac")) {
    model = Hypervisor::apple;
  }
  if (!cpuid) {
    return VirtualMachine{.hypervisor = model.value_or(Hypervisor::unknown)};
  }
  return internal::classify_vm(cpuid, model);
}

}  // namespace

// macOS has no native containers: Docker Desktop and similar run Linux containers in a VM.
result<Virtualization> virtualization() { return Virtualization{.vm = virtual_machine(), .container = std::nullopt}; }

}  // namespace hwinfo

#endif  // HWINFO_APPLE
