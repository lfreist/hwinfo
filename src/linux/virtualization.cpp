// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/virtualization.h>

#include <cstdlib>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "internal/dmi.h"
#include "internal/file.h"
#include "internal/smbios.h"
#include "internal/virtualization.h"

namespace hwinfo {

namespace {

bool exists(const std::filesystem::path& path) {
  std::error_code ec;
  return std::filesystem::exists(path, ec);
}

std::string read_or_empty(const std::filesystem::path& path) { return internal::read_file(path).value_or(""); }

bool xen_control_domain() { return read_or_empty("/proc/xen/capabilities").contains("control_d"); }

std::optional<Hypervisor> hypervisor_from_firmware() {
  if (const auto dmi = internal::dmi_directory()) {
    const auto vendor = internal::read_attribute(*dmi / "sys_vendor").value_or("");
    const auto product = internal::read_attribute(*dmi / "product_name").value_or("");
    if (const auto hypervisor = internal::hypervisor_from_dmi(vendor, product)) {
      return hypervisor;
    }
  }
  if (!xen_control_domain() && (internal::read_attribute("/sys/hypervisor/type") == "xen" ||
                                read_or_empty("/proc/device-tree/hypervisor/compatible").contains("xen"))) {
    return Hypervisor::xen;
  }
  if (read_or_empty("/proc/device-tree/compatible").contains("linux,dummy-virt")) {
    return Hypervisor::qemu;
  }
  return std::nullopt;
}

std::optional<VirtualMachine> virtual_machine() {
  std::optional<internal::CpuidHypervisor> cpuid;
#ifdef HWINFO_X86
  cpuid = internal::cpuid_hypervisor();
  if (cpuid && cpuid->vendor_id == "XenVMMXenVMM" && xen_control_domain()) {
    cpuid->root_partition = true;
  }
#endif
  // the raw SMBIOS table is only readable by root
  const auto table = smbios::read_system_table();
  return internal::classify_vm(cpuid, hypervisor_from_firmware(), table && internal::smbios_vm_flag(*table));
}

std::optional<std::string> pid1_container() {
  const auto env = internal::read_file("/proc/1/environ");
  if (!env) {
    return std::nullopt;
  }
  for (const auto entry : internal::split(std::string_view(*env), '\0')) {
    if (entry.starts_with("container=")) {
      return std::string(entry.substr(10));
    }
  }
  return std::nullopt;
}

std::optional<Container> container() {
  const auto containerenv = internal::read_file("/run/.containerenv");
  return internal::classify_container({
      .containerenv = containerenv ? std::optional(*containerenv) : std::nullopt,
      .dockerenv = exists("/.dockerenv"),
      .systemd_container = internal::read_attribute("/run/systemd/container"),
      .pid1_container = pid1_container(),
      .openvz = exists("/proc/vz") && !exists("/proc/bc"),
      .cgroup = read_or_empty("/proc/self/cgroup"),
      .mountinfo = read_or_empty("/proc/self/mountinfo"),
      .kernel_release = read_or_empty("/proc/sys/kernel/osrelease"),
      .kubernetes_env = std::getenv("KUBERNETES_SERVICE_HOST") != nullptr,
  });
}

}  // namespace

result<Virtualization> virtualization() { return Virtualization{.vm = virtual_machine(), .container = container()}; }

}  // namespace hwinfo

#endif  // HWINFO_UNIX
