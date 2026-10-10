// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/gpu.h>

#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <vector>

#include "internal/apple_cf.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/pci.h"
#include "pci.ids.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;

std::string_view pci_database() { return {reinterpret_cast<const char*>(pci_ids), pci_ids_size}; }

// The PCI device a GPU is attached to (Intel Macs, eGPUs).
// Apple Silicon GPUs are not PCI devices.
cf::io_ptr pci_device(io_registry_entry_t accelerator) {
  for (auto current = cf::parent(accelerator); current; current = cf::parent(current.get())) {
    if (IOObjectConformsTo(current.get(), "IOPCIDevice")) {
      return current;
    }
  }
  return {};
}

}  // namespace

// Every GPU has an IOAccelerator service (e.g. "AGXAcceleratorG13X", "IntelAccelerator",
// "AMDRadeonX6000_AMDNavi10GraphicsAccelerator").
result<std::vector<Gpu>> gpus(const GpuQuery& query) {
  const auto accelerators = cf::matching_services("IOAccelerator");
  if (!accelerators) {
    return std::unexpected(accelerators.error());
  }

  std::vector<Gpu> result;
  for (const auto& accelerator : *accelerators) {
    Gpu gpu{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = {},
        .name = {},
        .driver = cf::class_name(accelerator.get()),
    };
    if (const auto pci = pci_device(accelerator.get())) {
      // "vendor-id" / "device-id": 4 byte little endian data
      const auto vendor_id = cf::number_property<std::uint16_t>(pci.get(), CFSTR("vendor-id"));
      const auto device_id = cf::number_property<std::uint16_t>(pci.get(), CFSTR("device-id"));
      if (vendor_id && device_id) {
        const auto names = internal::lookup_pci(pci_database(), *vendor_id, *device_id);
        gpu.vendor = names.vendor.value_or(std::format("{:#06x}", *vendor_id));
        gpu.name = names.device.value_or(std::format("{:#06x}", *device_id));
        gpu.pci = PciDevice{
            .vendor_id = *vendor_id,
            .device_id = *device_id,
            .address = cf::string_property(pci.get(), CFSTR("pcidebug")).and_then(internal::parse_pcidebug),
        };
        gpu.type = internal::classify_pci_gpu(*gpu.pci);
        if (*vendor_id == internal::pci_vendor::amd) {
          gpu.type = GpuType::discrete;
        }
      }
      // marketing name, e.g. "AMD Radeon Pro 5500M"
      if (auto model = cf::string_property(pci.get(), CFSTR("model"))) {
        gpu.name = std::move(*model);
      }
      if (const auto vram_mb = cf::number_property<std::uint64_t>(pci.get(), CFSTR("VRAM,totalMB"))) {
        gpu.dedicated_memory = *vram_mb * ByteUnit::MiB;
      }
    } else {
      // Apple Silicon: integrated GPU using the unified memory
      gpu.vendor = "Apple";
      gpu.type = GpuType::integrated;
      gpu.name = cf::string_property(accelerator.get(), CFSTR("model")).value_or(std::string{});  // "Apple M1 Pro"
    }
    if (gpu.name.empty()) {
      gpu.name = gpu.driver.value_or(std::string{});
    }
    // Apple Silicon: an Apple "GPU core" is the equivalent of a compute unit, with 128 FP32 ALUs
    if (const auto cores = cf::number_property<std::uint32_t>(accelerator.get(), CFSTR("gpu-core-count"))) {
      gpu.compute_units = cores;
      gpu.cores = *cores * 128;
    }
    gpu.unified_memory = internal::unified_memory(gpu.type);
    result.push_back(std::move(gpu));
  }

  internal::gpu::enrich(result, query);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
