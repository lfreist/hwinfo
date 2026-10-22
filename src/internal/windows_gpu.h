// Copyright Leon Freist

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/gpu.h>

#include <algorithm>
#include <optional>
#include <string_view>
#include <vector>

#include "internal/gpu.h"
#include "internal/strings.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo::internal {

inline std::optional<PciDevice> video_controller_pci(std::string_view pnp_id) {
  if (!starts_with_ignore_case(pnp_id, "PCI\\")) {
    return std::nullopt;
  }
  std::optional<std::uint16_t> vendor;
  std::optional<std::uint16_t> device;
  for (const auto part : split(pnp_id.substr(4), '&')) {
    if (part.size() == 8 && starts_with_ignore_case(part, "VEN_")) {
      if (const auto id = parse<std::uint16_t>(part.substr(4), 16)) {
        vendor = *id;
      }
    } else if (part.size() >= 8 && starts_with_ignore_case(part, "DEV_") && (part.size() == 8 || part[8] == '\\')) {
      if (const auto id = parse<std::uint16_t>(part.substr(4, 4), 16)) {
        device = *id;
      }
    }
  }
  if (!vendor || !device) {
    return std::nullopt;
  }
  return PciDevice{.vendor_id = *vendor, .device_id = *device};
}

inline Gpu video_controller(const wmi::Row& row) {
  Gpu gpu{
      .vendor = row.string("AdapterCompatibility").value_or(""),
      .name = row.string("Name").value_or(""),
      .driver_version = row.string("DriverVersion"),
      .pci = video_controller_pci(row.string("PNPDeviceID").value_or("")),
  };
  if (const auto memory = row.number<std::uint32_t>("AdapterRAM")) {
    gpu.dedicated_memory = Bytes{*memory};
  }
  if (gpu.pci) {
    gpu.type = classify_pci_gpu(*gpu.pci);
    gpu.unified_memory = unified_memory(gpu.type);
  }
  return gpu;
}

// Match each WMI controller at most once, including systems with multiple identical cards.
inline auto match_video_controller(std::vector<Gpu>& gpus, const Gpu& adapter) {
  const auto compatible = [&](const Gpu& gpu) {
    return !gpu.luid &&
           (!gpu.pci || !adapter.pci ||
            (gpu.pci->vendor_id == adapter.pci->vendor_id && gpu.pci->device_id == adapter.pci->device_id));
  };
  const auto named = std::ranges::find_if(
      gpus, [&](const Gpu& gpu) { return compatible(gpu) && !gpu.name.empty() && gpu.name == adapter.name; });
  if (named != gpus.end()) {
    return named;
  }
  auto found = gpus.end();
  for (auto it = gpus.begin(); it != gpus.end(); ++it) {
    if (compatible(*it) && it->pci && adapter.pci) {
      if (found != gpus.end()) {
        return gpus.end();  // PCI ids alone do not distinguish identical cards.
      }
      found = it;
    }
  }
  return found;
}

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
