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
#include "internal/pci.h"
#include "pci.ids.h"

#ifdef USE_OCL
#include "opencl/device.h"
#endif

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

#ifdef USE_OCL
void add_opencl_info(std::vector<Gpu>& gpus) {
  for (auto* cl_gpu : opencl_::DeviceManager::get_list<opencl_::Filter::GPU>()) {
    for (auto& gpu : gpus) {
      if (!cl_gpu->name().contains(gpu.name)) {
        continue;
      }
      gpu.driver_version = cl_gpu->driver_version();
      gpu.frequency = cl_gpu->clock_frequency_MHz() * FrequencyUnit::MHz;
      gpu.cores = static_cast<std::uint32_t>(cl_gpu->cores());
      if (!gpu.dedicated_memory) {
        gpu.dedicated_memory = Bytes{cl_gpu->memory_Bytes()};
      }
    }
  }
}
#endif

}  // namespace

// Every GPU has an IOAccelerator service (e.g. "AGXAcceleratorG13X", "IntelAccelerator",
// "AMDRadeonX6000_AMDNavi10GraphicsAccelerator").
result<std::vector<Gpu>> gpus() {
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
        gpu.pci = PciId{*vendor_id, *device_id};
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
      gpu.name = cf::string_property(accelerator.get(), CFSTR("model")).value_or(std::string{});  // "Apple M1 Pro"
    }
    if (gpu.name.empty()) {
      gpu.name = gpu.driver.value_or(std::string{});
    }
    gpu.cores = cf::number_property<std::uint32_t>(accelerator.get(), CFSTR("gpu-core-count"));  // Apple Silicon
    result.push_back(std::move(gpu));
  }

#ifdef USE_OCL
  add_opencl_info(result);
#endif
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
