// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace hwinfo {

enum class Hypervisor {
  unknown,
  kvm,
  qemu,  // QEMU without hardware acceleration (TCG)
  vmware,
  hyper_v,
  xen,
  virtualbox,
  parallels,
  bhyve,
  apple,   // Apple Virtualization framework
  amazon,  // Amazon EC2 (Nitro)
  google,  // Google Compute Engine
};

enum class ContainerRuntime { unknown, docker, podman, containerd, cri_o, lxc, systemd_nspawn, openvz, wsl };

struct VirtualMachine {
  Hypervisor hypervisor = Hypervisor::unknown;
  std::optional<std::string> vendor_id{};  // raw CPUID hypervisor vendor (x86 only), e.g. "KVMKVMKVM", "Microsoft Hv"

  friend bool operator==(const VirtualMachine&, const VirtualMachine&) = default;
};

struct Container {
  ContainerRuntime runtime = ContainerRuntime::unknown;
  bool kubernetes = false;

  friend bool operator==(const Container&, const Container&) = default;
};

// The environment the calling process runs in. A VM and a container can be stacked, e.g. a Docker container on a cloud
// VM or WSL 2 (a Linux distro in a Hyper-V VM).
//
// Detection is heuristic: std::nullopt means "nothing detected", not a guarantee of bare metal, and an unknown
// hypervisor or runtime is a regular result. Nested VMs are invisible: `vm` is the hypervisor directly below the OS.
// Containers are detected on Linux (and Windows containers on Windows); macOS has no native containers.
// A Windows host with Hyper-V or virtualization-based security enabled runs on the hypervisor as well, but is reported
// as bare metal.
struct Virtualization {
  std::optional<VirtualMachine> vm{};
  std::optional<Container> container{};

  friend bool operator==(const Virtualization&, const Virtualization&) = default;
};

[[nodiscard]] HWINFO_API result<Virtualization> virtualization();

constexpr std::string_view to_string(Hypervisor hypervisor) noexcept {
  switch (hypervisor) {
    case Hypervisor::kvm:
      return "KVM";
    case Hypervisor::qemu:
      return "QEMU";
    case Hypervisor::vmware:
      return "VMware";
    case Hypervisor::hyper_v:
      return "Hyper-V";
    case Hypervisor::xen:
      return "Xen";
    case Hypervisor::virtualbox:
      return "VirtualBox";
    case Hypervisor::parallels:
      return "Parallels";
    case Hypervisor::bhyve:
      return "bhyve";
    case Hypervisor::apple:
      return "Apple Virtualization";
    case Hypervisor::amazon:
      return "Amazon EC2";
    case Hypervisor::google:
      return "Google Compute Engine";
    case Hypervisor::unknown:
      break;
  }
  return "unknown";
}

constexpr std::string_view to_string(ContainerRuntime runtime) noexcept {
  switch (runtime) {
    case ContainerRuntime::docker:
      return "Docker";
    case ContainerRuntime::podman:
      return "Podman";
    case ContainerRuntime::containerd:
      return "containerd";
    case ContainerRuntime::cri_o:
      return "CRI-O";
    case ContainerRuntime::lxc:
      return "LXC";
    case ContainerRuntime::systemd_nspawn:
      return "systemd-nspawn";
    case ContainerRuntime::openvz:
      return "OpenVZ";
    case ContainerRuntime::wsl:
      return "WSL";
    case ContainerRuntime::unknown:
      break;
  }
  return "unknown";
}

// e.g. "none detected", "KVM VM", "Docker container on Amazon EC2 VM", "containerd container (Kubernetes)"
inline std::string to_string(const Virtualization& virtualization) {
  std::string out;
  if (const auto& container = virtualization.container) {
    out = std::format("{} container", to_string(container->runtime));
    if (container->kubernetes) {
      out += " (Kubernetes)";
    }
  }
  if (const auto& vm = virtualization.vm) {
    out += std::format("{}{} VM", out.empty() ? "" : " on ", to_string(vm->hypervisor));
  }
  return out.empty() ? "none detected" : out;
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Hypervisor> : hwinfo::detail::to_string_formatter<hwinfo::Hypervisor> {};

template <>
struct std::formatter<hwinfo::ContainerRuntime> : hwinfo::detail::to_string_formatter<hwinfo::ContainerRuntime> {};

template <>
struct std::formatter<hwinfo::Virtualization> : hwinfo::detail::to_string_formatter<hwinfo::Virtualization> {};
