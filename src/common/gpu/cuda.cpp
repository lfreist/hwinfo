// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// CUDA driver API backend: complements NVML with the SM count, L2 cache size and the integrated flag.

#include "internal/gpu_api/cuda.h"

#include <array>
#include <optional>
#include <string>

#include "internal/dynamic_library.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/pci.h"

namespace hwinfo::internal::gpu {

namespace {

using namespace internal::cuda;

struct Cuda {
  cuDriverGetVersion_t driver_get_version = nullptr;
  cuDeviceGetCount_t device_get_count = nullptr;
  cuDeviceGet_t device_get = nullptr;
  cuDeviceGetName_t device_get_name = nullptr;
  cuDeviceGetAttribute_t device_get_attribute = nullptr;
  cuDeviceGetUuid_t device_get_uuid = nullptr;
  cuDeviceTotalMem_v2_t device_total_mem = nullptr;
  cuDeviceGetLuid_t device_get_luid = nullptr;  // Windows only
};

const Cuda* cuda() {
  static const std::optional<Cuda> loaded = []() -> std::optional<Cuda> {
    const auto lib = DynamicLibrary::open({
#ifdef HWINFO_WINDOWS
        "nvcuda.dll",
#else
        "libcuda.so.1",
        "libcuda.so",
#endif
    });
    cuInit_t init = nullptr;
    Cuda api;
    if (!lib.load(init, "cuInit") || !lib.load(api.driver_get_version, "cuDriverGetVersion") ||
        !lib.load(api.device_get_count, "cuDeviceGetCount") || !lib.load(api.device_get, "cuDeviceGet") ||
        !lib.load(api.device_get_attribute, "cuDeviceGetAttribute")) {
      return std::nullopt;
    }
    if (init(0) != CUDA_SUCCESS) {
      return std::nullopt;  // e.g. CUDA_ERROR_NO_DEVICE
    }
    lib.load(api.device_get_name, "cuDeviceGetName");
    if (!lib.load(api.device_get_uuid, "cuDeviceGetUuid_v2")) {
      lib.load(api.device_get_uuid, "cuDeviceGetUuid");
    }
    lib.load(api.device_total_mem, "cuDeviceTotalMem_v2");
    lib.load(api.device_get_luid, "cuDeviceGetLuid");
    return api;
  }();
  return loaded ? &*loaded : nullptr;
}

std::optional<int> attribute(const Cuda& api, CUdevice device, CUdevice_attribute attribute) {
  int value = 0;
  if (api.device_get_attribute(&value, attribute, device) != CUDA_SUCCESS) {
    return std::nullopt;
  }
  return value;
}

std::optional<std::uint32_t> positive(std::optional<int> value) {
  return value && *value > 0 ? std::optional(static_cast<std::uint32_t>(*value)) : std::nullopt;
}

Gpu read_device(const Cuda& api, CUdevice device, const GpuApi& version) {
  Gpu gpu;
  if (std::array<char, 256> name{};
      api.device_get_name && api.device_get_name(name.data(), static_cast<int>(name.size()), device) == CUDA_SUCCESS) {
    gpu.name = name.data();
  }
  if (CUuuid uuid{}; api.device_get_uuid && api.device_get_uuid(&uuid, device) == CUDA_SUCCESS) {
    gpu.uuid.emplace();
    for (std::size_t i = 0; i < gpu.uuid->bytes.size(); ++i) {
      gpu.uuid->bytes[i] = static_cast<std::uint8_t>(uuid.bytes[i]);
    }
  }
  std::array<char, 8> luid{};
  unsigned int node_mask = 0;
  if (api.device_get_luid && api.device_get_luid(luid.data(), &node_mask, device) == CUDA_SUCCESS) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < luid.size(); ++i) {
      value |= std::uint64_t{static_cast<std::uint8_t>(luid[i])} << (8 * i);
    }
    gpu.luid = value;
  }
  const auto bus = attribute(api, device, CU_DEVICE_ATTRIBUTE_PCI_BUS_ID);
  const auto slot = attribute(api, device, CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID);
  if (bus && slot) {
    const auto domain = attribute(api, device, CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID).value_or(0);
    gpu.pci = PciDevice{
        .vendor_id = pci_vendor::nvidia,
        .address = format_pci_address(static_cast<std::uint32_t>(domain), static_cast<std::uint32_t>(*bus),
                                      static_cast<std::uint32_t>(*slot), 0),
    };
  }

  const bool integrated = attribute(api, device, CU_DEVICE_ATTRIBUTE_INTEGRATED).value_or(0) != 0;
  gpu.type = integrated ? GpuType::integrated : GpuType::discrete;
  gpu.compute_units = positive(attribute(api, device, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT));
  const auto major = attribute(api, device, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR);
  const auto minor = attribute(api, device, CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR);
  if (major && minor) {
    apply_compute_capability(gpu, *major, *minor);
  }
  if (const auto l2 = positive(attribute(api, device, CU_DEVICE_ATTRIBUTE_L2_CACHE_SIZE))) {
    gpu.l2_cache = Bytes{*l2};
  }
  gpu.memory_bus_width = positive(attribute(api, device, CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH));
  if (const auto khz = positive(attribute(api, device, CU_DEVICE_ATTRIBUTE_CLOCK_RATE))) {
    gpu.max_frequency = std::uint64_t{*khz} * FrequencyUnit::kHz;
  }
  if (const auto khz = positive(attribute(api, device, CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE))) {
    gpu.max_memory_frequency = std::uint64_t{*khz} * FrequencyUnit::kHz;
  }
  if (std::size_t bytes = 0;
      !integrated && api.device_total_mem && api.device_total_mem(&bytes, device) == CUDA_SUCCESS) {
    gpu.dedicated_memory = Bytes{bytes};
  }
  gpu.compute_apis.push_back(version);
  return gpu;
}

}  // namespace

BackendResult cuda_devices() {
  const Cuda* api = cuda();
  int count = 0;
  int version = 0;
  if (api == nullptr || api->device_get_count(&count) != CUDA_SUCCESS ||
      api->driver_get_version(&version) != CUDA_SUCCESS) {
    return {};
  }
  const GpuApi cuda_api{.name = "CUDA", .version = cuda_version(version)};
  BackendResult result;
  for (int i = 0; i < count; ++i) {
    CUdevice device = 0;
    if (api->device_get(&device, i) == CUDA_SUCCESS) {
      result.push_back(read_device(*api, device, cuda_api));
    }
  }
  return result;
}

}  // namespace hwinfo::internal::gpu
