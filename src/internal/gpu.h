// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Platform independent rules to classify GPUs as integrated, discrete or virtual. Not part of the public API.

#pragma once

#include <hwinfo/gpu.h>

#include <cstdint>
#include <optional>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal {

namespace pci_vendor {
constexpr std::uint16_t intel = 0x8086;
constexpr std::uint16_t amd = 0x1002;
constexpr std::uint16_t nvidia = 0x10de;
}  // namespace pci_vendor

// Vendors whose display adapters only exist in virtual machines.
constexpr bool is_virtual_gpu_vendor(std::uint16_t vendor_id) noexcept {
  switch (vendor_id) {
    case 0x1234:  // QEMU standard VGA (bochs)
    case 0x1414:  // Microsoft Hyper-V
    case 0x15ad:  // VMware SVGA
    case 0x1af4:  // Red Hat virtio-gpu
    case 0x1b36:  // Red Hat QXL
    case 0x80ee:  // VirtualBox
      return true;
    default:
      return false;
  }
}

// Bus number of a PCI address "domain:bus:device.function".
inline std::optional<std::uint32_t> pci_bus(std::string_view address) {
  const auto first = address.find(':');
  const auto second = address.find(':', first + 1);
  if (first == std::string_view::npos || second == std::string_view::npos) {
    return std::nullopt;
  }
  const auto bus = parse<std::uint32_t>(address.substr(first + 1, second - first - 1), 16);
  return bus ? std::optional(*bus) : std::nullopt;
}

// Classification from the PCI identity alone, for when the platform has no better source:
// - Intel integrated GPUs always sit on the root bus (0000:00:02.0); Intel Arc cards are behind PCIe bridges.
// - NVIDIA only makes discrete PCI GPUs (integrated Tegra GPUs are not PCI devices).
// - AMD APUs and discrete Radeons cannot be told apart by the PCI identity: unknown.
inline GpuType classify_pci_gpu(const PciDevice& pci) {
  if (is_virtual_gpu_vendor(pci.vendor_id)) {
    return GpuType::virtualized;
  }
  switch (pci.vendor_id) {
    case pci_vendor::intel: {
      const auto bus = pci.address ? pci_bus(*pci.address) : std::nullopt;
      if (!bus) {
        return GpuType::unknown;
      }
      return *bus == 0 ? GpuType::integrated : GpuType::discrete;
    }
    case pci_vendor::nvidia:
      return GpuType::discrete;
    default:
      return GpuType::unknown;
  }
}

constexpr std::optional<bool> unified_memory(GpuType type) noexcept {
  switch (type) {
    case GpuType::integrated:
      return true;
    case GpuType::discrete:
      return false;
    default:
      return std::nullopt;
  }
}

}  // namespace hwinfo::internal
