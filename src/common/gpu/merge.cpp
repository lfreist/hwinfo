// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "internal/gpu.h"
#include "internal/gpu_backend.h"

namespace hwinfo::internal::gpu {

namespace {

std::uint16_t vendor_id(const Gpu& gpu) { return gpu.pci ? gpu.pci->vendor_id : 0; }
std::uint16_t device_id(const Gpu& gpu) { return gpu.pci ? gpu.pci->device_id : 0; }
const std::optional<std::string>& address(const Gpu& gpu) {
  static const std::optional<std::string> none;
  return gpu.pci ? gpu.pci->address : none;
}

template <typename T>
bool both_differ(const std::optional<T>& a, const std::optional<T>& b) {
  return a && b && *a != *b;
}

// Two descriptions certainly belong to different devices.
bool conflicts(const Gpu& a, const Gpu& b) {
  const bool vendors_differ = vendor_id(a) != 0 && vendor_id(b) != 0 && vendor_id(a) != vendor_id(b);
  const bool devices_differ = device_id(a) != 0 && device_id(b) != 0 && device_id(a) != device_id(b);
  return vendors_differ || devices_differ || both_differ(address(a), address(b)) || both_differ(a.luid, b.luid) ||
         both_differ(a.uuid, b.uuid);
}

template <typename T>
bool both_equal(const std::optional<T>& a, const std::optional<T>& b) {
  return a && b && *a == *b;
}

// The single OS GPU fulfilling `predicate` (and not conflicting with `device`), if exactly one does.
template <typename Predicate>
std::optional<std::size_t> unique(const std::vector<Gpu>& gpus, const Gpu& device, Predicate predicate) {
  std::optional<std::size_t> found;
  for (std::size_t i = 0; i < gpus.size(); ++i) {
    if (conflicts(gpus[i], device) || !predicate(gpus[i])) {
      continue;
    }
    if (found) {
      return std::nullopt;  // ambiguous
    }
    found = i;
  }
  return found;
}

std::optional<std::size_t> find_match(const std::vector<Gpu>& gpus, const Gpu& device) {
  // unique identifiers
  for (std::size_t i = 0; i < gpus.size(); ++i) {
    if (both_equal(address(gpus[i]), address(device)) || both_equal(gpus[i].luid, device.luid) ||
        both_equal(gpus[i].uuid, device.uuid)) {
      return i;
    }
  }
  // PCI ids, then vendor alone
  if (vendor_id(device) != 0 && device_id(device) != 0) {
    if (auto i = unique(gpus, device, [&](const Gpu& gpu) {
          return vendor_id(gpu) == vendor_id(device) && device_id(gpu) == device_id(device);
        })) {
      return i;
    }
  }
  if (vendor_id(device) != 0) {
    return unique(gpus, device, [&](const Gpu& gpu) { return vendor_id(gpu) == vendor_id(device); });
  }
  return std::nullopt;
}

// Identifiers learned from a matched backend device, so that lower priority backends can match by them, too. The
// compute APIs are collected here as well, to list them in priority order.
void absorb_keys(Gpu& gpu, const Gpu& device) {
  for (const auto& api : device.compute_apis) {
    if (!std::ranges::contains(gpu.compute_apis, api)) {
      gpu.compute_apis.push_back(api);
    }
  }
  if (!gpu.uuid) {
    gpu.uuid = device.uuid;
  }
  if (!gpu.luid) {
    gpu.luid = device.luid;
  }
  if (gpu.pci && !gpu.pci->address && device.pci) {
    gpu.pci->address = device.pci->address;
  }
}

template <typename T>
void take(std::optional<T>& target, const std::optional<T>& source) {
  if (source) {
    target = source;
  }
}

void overlay(Gpu& gpu, const Gpu& device) {
  if (!device.name.empty()) {
    gpu.name = device.name;
  }
  if (gpu.vendor.empty()) {
    gpu.vendor = device.vendor;
  }
  // a virtual adapter stays virtual even if a compute driver reports it as integrated or discrete
  if (device.type != GpuType::unknown && gpu.type != GpuType::virtualized) {
    gpu.type = device.type;
    gpu.unified_memory = unified_memory(device.type);
  }
  take(gpu.unified_memory, device.unified_memory);
  take(gpu.uuid, device.uuid);
  take(gpu.luid, device.luid);
  take(gpu.architecture, device.architecture);
  take(gpu.compute_capability, device.compute_capability);
  take(gpu.compute_units, device.compute_units);
  take(gpu.cores, device.cores);
  take(gpu.driver, device.driver);
  take(gpu.driver_version, device.driver_version);
  take(gpu.vbios_version, device.vbios_version);
  take(gpu.serial, device.serial);
  take(gpu.dedicated_memory, device.dedicated_memory);
  take(gpu.shared_memory, device.shared_memory);
  take(gpu.memory_type, device.memory_type);
  take(gpu.memory_bus_width, device.memory_bus_width);
  take(gpu.l2_cache, device.l2_cache);
  take(gpu.max_frequency, device.max_frequency);
  take(gpu.max_memory_frequency, device.max_memory_frequency);
  take(gpu.power_limit, device.power_limit);
  if (device.pci) {
    if (!gpu.pci) {
      gpu.pci = device.pci;
    } else {
      take(gpu.pci->address, device.pci->address);
      take(gpu.pci->max_link, device.pci->max_link);
    }
  }
}

}  // namespace

void merge(std::vector<Gpu>& gpus, std::span<const BackendResult> results) {
  // 1. match, highest priority first: keys learned from earlier backends help later ones
  std::vector<std::vector<std::optional<std::size_t>>> matches(results.size());
  for (std::size_t r = 0; r < results.size(); ++r) {
    for (const auto& device : results[r]) {
      const auto match = find_match(gpus, device);
      if (match) {
        absorb_keys(gpus[*match], device);
      }
      matches[r].push_back(match);
    }
  }
  // 2. overlay, lowest priority first: higher priority backends overwrite
  for (std::size_t r = results.size(); r-- > 0;) {
    for (std::size_t d = 0; d < results[r].size(); ++d) {
      if (const auto match = matches[r][d]) {
        overlay(gpus[*match], results[r][d]);
      }
    }
  }
}

}  // namespace hwinfo::internal::gpu
