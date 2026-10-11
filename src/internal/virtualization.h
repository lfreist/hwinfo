// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// VM and container detection heuristics shared by the platform implementations. Pure functions on the gathered
// signals, so they can be unit tested on any platform. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>
#include <hwinfo/virtualization.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

#include "procfs.h"
#include "strings.h"

#ifdef HWINFO_X86
#include "cpuid.h"
#endif

namespace hwinfo::internal {

// ----- Virtual machines ---------------------------------------------------------------------------------------------

// Hypervisor of a CPUID hypervisor vendor signature (leaf 0x40000000).
constexpr std::optional<Hypervisor> hypervisor_from_vendor_id(std::string_view id) noexcept {
  if (id == "KVMKVMKVM" || id == "Linux KVM Hv") {
    return Hypervisor::kvm;
  }
  if (id == "TCGTCGTCGTCG") {
    return Hypervisor::qemu;
  }
  if (id == "Microsoft Hv") {
    return Hypervisor::hyper_v;
  }
  if (id == "VMwareVMware") {
    return Hypervisor::vmware;
  }
  if (id == "XenVMMXenVMM") {
    return Hypervisor::xen;
  }
  if (id == "VBoxVBoxVBox") {
    return Hypervisor::virtualbox;
  }
  if (id == "prl hyperv" || id == "lrpepyh  vr") {
    return Hypervisor::parallels;
  }
  if (id == "bhyve bhyve") {
    return Hypervisor::bhyve;
  }
  return std::nullopt;
}

// Hypervisor of the SMBIOS system manufacturer and product name. Only matches values that are specific to VMs: e.g. a
// bare "Microsoft Corporation" is a Surface device and "Google" a Chromebook.
constexpr std::optional<Hypervisor> hypervisor_from_dmi(std::string_view vendor, std::string_view product) noexcept {
  if (equals_ignore_case(vendor, "QEMU") || equals_ignore_case(vendor, "Bochs")) {
    return Hypervisor::qemu;
  }
  if (equals_ignore_case(vendor, "KubeVirt") || equals_ignore_case(product, "KVM") ||
      starts_with_ignore_case(product, "OpenStack")) {
    return Hypervisor::kvm;
  }
  if (starts_with_ignore_case(vendor, "VMware")) {
    return Hypervisor::vmware;
  }
  if (equals_ignore_case(vendor, "innotek GmbH") || equals_ignore_case(product, "VirtualBox")) {
    return Hypervisor::virtualbox;
  }
  if (equals_ignore_case(vendor, "Xen")) {
    return Hypervisor::xen;
  }
  if (starts_with_ignore_case(vendor, "Parallels")) {
    return Hypervisor::parallels;
  }
  if (equals_ignore_case(vendor, "BHYVE") || equals_ignore_case(product, "BHYVE")) {
    return Hypervisor::bhyve;
  }
  if (equals_ignore_case(vendor, "Microsoft Corporation") && equals_ignore_case(product, "Virtual Machine")) {
    return Hypervisor::hyper_v;
  }
  if (equals_ignore_case(vendor, "Amazon EC2") && !product.ends_with(".metal")) {  // e.g. "c5.metal": bare metal
    return Hypervisor::amazon;
  }
  if (equals_ignore_case(vendor, "Google") && equals_ignore_case(product, "Google Compute Engine")) {
    return Hypervisor::google;
  }
  if (equals_ignore_case(vendor, "Apple Inc.") && contains_ignore_case(product, "Virtual")) {
    return Hypervisor::apple;
  }
  return std::nullopt;
}

struct CpuidHypervisor {
  std::string vendor_id{};      // leaf 0x40000000
  std::string secondary_id{};   // leaf 0x40000100: the actual hypervisor behind a Hyper-V compatible interface
  bool root_partition = false;  // the OS is the Hyper-V root partition (host) or the Xen control domain (dom0)
};

#ifdef HWINFO_X86
// std::nullopt if the CPUID hypervisor bit is not set.
inline std::optional<CpuidHypervisor> cpuid_hypervisor() {
  if ((cpuid(1).ecx & (1u << 31)) == 0) {
    return std::nullopt;
  }
  const auto base = cpuid(0x40000000);
  CpuidHypervisor hypervisor{.vendor_id = cpuid_vendor(base)};
  if (hypervisor.vendor_id == "Microsoft Hv") {
    // KVM, Xen, VirtualBox, ... offer a Hyper-V compatible interface to Windows guests and move their own one up
    hypervisor.secondary_id = cpuid_vendor(cpuid(0x40000100));
    // the CreatePartitions privilege is only granted to the root partition
    hypervisor.root_partition = base.eax >= 0x40000003 && (cpuid(0x40000003).ebx & 1u) != 0;
  }
  return hypervisor;
}
#endif

// Combines the CPUID and SMBIOS based detection. Either may be missing (non-x86, no SMBIOS).
inline std::optional<VirtualMachine> classify_vm(const std::optional<CpuidHypervisor>& cpuid,
                                                 std::optional<Hypervisor> dmi) {
  if (cpuid && cpuid->root_partition) {
    return std::nullopt;
  }
  if (!cpuid && !dmi) {
    return std::nullopt;
  }
  VirtualMachine vm;
  std::optional<Hypervisor> from_cpuid;
  if (cpuid) {
    vm.vendor_id = non_empty(cpuid->vendor_id);
    from_cpuid = hypervisor_from_vendor_id(cpuid->secondary_id);
    if (from_cpuid) {
      vm.vendor_id = cpuid->secondary_id;
    } else {
      from_cpuid = hypervisor_from_vendor_id(cpuid->vendor_id);
    }
  }
  // SMBIOS knows the product (cloud, VirtualBox with a KVM interface, ...), except that QEMU accelerated by KVM reports
  // "QEMU" there as well
  if (dmi && (*dmi != Hypervisor::qemu || !from_cpuid)) {
    vm.hypervisor = *dmi;
  } else {
    vm.hypervisor = from_cpuid.value_or(Hypervisor::unknown);
  }
  return vm;
}

// ----- Containers ---------------------------------------------------------------------------------------------------

// Runtime of a container manager name as used by systemd (/run/systemd/container, `container=` environment variable).
constexpr ContainerRuntime container_runtime_from_name(std::string_view name) noexcept {
  name = trim(name);
  if (name == "docker") {
    return ContainerRuntime::docker;
  }
  if (name == "podman") {
    return ContainerRuntime::podman;
  }
  if (name == "containerd") {
    return ContainerRuntime::containerd;
  }
  if (name == "crio" || name == "cri-o") {
    return ContainerRuntime::cri_o;
  }
  if (name == "lxc" || name == "lxc-libvirt") {
    return ContainerRuntime::lxc;
  }
  if (name == "systemd-nspawn") {
    return ContainerRuntime::systemd_nspawn;
  }
  if (name == "openvz") {
    return ContainerRuntime::openvz;
  }
  if (name == "wsl") {
    return ContainerRuntime::wsl;
  }
  return ContainerRuntime::unknown;
}

// Runtime from /run/.containerenv, written by Podman and CRI-O. Its content (`engine="podman-5.0.2"`) may be empty.
inline ContainerRuntime container_runtime_from_containerenv(std::string_view content) {
  const auto values = procfs::parse_os_release(content);
  if (const auto engine = values.find("engine"); engine != values.end()) {
    if (starts_with_ignore_case(engine->second, "cri-o") || starts_with_ignore_case(engine->second, "crio")) {
      return ContainerRuntime::cri_o;
    }
  }
  return ContainerRuntime::podman;
}

struct CgroupContainer {
  std::optional<ContainerRuntime> runtime{};
  bool kubernetes = false;
};

// Container hints in /proc/self/cgroup. Only cgroup v1 or a v2 hierarchy without cgroup namespace reveal the path
// outside of the container, e.g. "0::/system.slice/docker-<id>.scope" or "4:memory:/kubepods/burstable/pod<uid>/<id>".
inline CgroupContainer container_from_cgroup(std::string_view content) {
  CgroupContainer result;
  for (const auto line : lines(content)) {
    const auto path_start = line.find(":/");
    if (path_start == std::string_view::npos || line.find(':') == path_start) {
      continue;
    }
    for (const auto component : split(line.substr(path_start + 2), '/')) {
      if (starts_with_ignore_case(component, "kubepods")) {
        result.kubernetes = true;
      } else if (result.runtime) {
        continue;
      } else if (component == "docker" || (component.starts_with("docker-") && component.ends_with(".scope"))) {
        result.runtime = ContainerRuntime::docker;
      } else if (component.starts_with("libpod-")) {
        result.runtime = ContainerRuntime::podman;
      } else if (component.starts_with("crio-")) {
        result.runtime = ContainerRuntime::cri_o;
      } else if (component.starts_with("cri-containerd-")) {
        result.runtime = ContainerRuntime::containerd;
      } else if (component == "lxc" || component.starts_with("lxc.payload")) {
        result.runtime = ContainerRuntime::lxc;
      }
    }
  }
  return result;
}

// Runtime from the storage of an overlay root filesystem in /proc/self/mountinfo, e.g.
// "... / / rw,relatime - overlay overlay rw,lowerdir=/var/lib/docker/overlay2/l/...".
inline std::optional<ContainerRuntime> container_from_mountinfo(std::string_view content) {
  std::optional<ContainerRuntime> runtime;
  for (const auto& mount : procfs::parse_mountinfo(content)) {
    if (mount.mount_point != "/") {
      continue;
    }
    runtime.reset();  // the last mount on "/" is the visible one
    if (mount.fs_type != "overlay") {
      continue;
    }
    if (mount.super_options.contains("/var/lib/docker/")) {
      runtime = ContainerRuntime::docker;
    } else if (mount.super_options.contains("/containerd/")) {
      runtime = ContainerRuntime::containerd;
    } else if (mount.super_options.contains("/containers/storage/")) {
      runtime = ContainerRuntime::podman;
    }
  }
  return runtime;
}

// Everything a Linux system reveals about the container it runs in.
struct LinuxContainerSignals {
  std::optional<std::string> containerenv{};       // content of /run/.containerenv
  bool dockerenv = false;                          // /.dockerenv exists
  std::optional<std::string> systemd_container{};  // content of /run/systemd/container
  std::optional<std::string> pid1_container{};     // `container=` in /proc/1/environ (usually requires root)
  bool openvz = false;                             // /proc/vz exists, but not /proc/bc
  std::string cgroup{};                            // content of /proc/self/cgroup
  std::string mountinfo{};                         // content of /proc/self/mountinfo
  std::string kernel_release{};                    // /proc/sys/kernel/osrelease
  bool kubernetes_env = false;                     // KUBERNETES_SERVICE_HOST is set
};

inline std::optional<Container> classify_container(const LinuxContainerSignals& signals) {
  const auto cgroup = container_from_cgroup(signals.cgroup);
  const bool kubernetes = signals.kubernetes_env || cgroup.kubernetes;

  std::optional<ContainerRuntime> runtime;
  if (signals.containerenv) {
    runtime = container_runtime_from_containerenv(*signals.containerenv);
  } else if (signals.dockerenv) {
    runtime = ContainerRuntime::docker;
  } else if (signals.systemd_container) {
    runtime = container_runtime_from_name(*signals.systemd_container);
  } else if (signals.pid1_container) {
    runtime = container_runtime_from_name(*signals.pid1_container);
  } else if (signals.openvz) {
    runtime = ContainerRuntime::openvz;
  } else if (cgroup.runtime) {
    runtime = cgroup.runtime;
  } else if (const auto from_mounts = container_from_mountinfo(signals.mountinfo)) {
    runtime = from_mounts;
  } else if (contains_ignore_case(signals.kernel_release, "microsoft")) {
    runtime = ContainerRuntime::wsl;  // checked last: Docker Desktop runs its containers on a WSL kernel as well
  } else if (kubernetes) {
    runtime = ContainerRuntime::unknown;
  }

  if (!runtime) {
    return std::nullopt;
  }
  return Container{.runtime = *runtime, .kubernetes = kubernetes};
}

}  // namespace hwinfo::internal
