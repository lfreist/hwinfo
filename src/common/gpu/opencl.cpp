// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// OpenCL backend: cross-vendor fallback for compute units, clocks, memory and the PCI location / UUID of a device.

#include "internal/gpu_api/opencl.h"

#include <array>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/dynamic_library.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/pci.h"
#include "internal/strings.h"

namespace hwinfo::internal::gpu {

namespace {

using namespace internal::cl;

struct OpenCl {
  clGetPlatformIDs_t get_platform_ids = nullptr;
  clGetPlatformInfo_t get_platform_info = nullptr;
  clGetDeviceIDs_t get_device_ids = nullptr;
  clGetDeviceInfo_t get_device_info = nullptr;

  explicit operator bool() const noexcept { return get_device_info != nullptr; }
};

const OpenCl& opencl() {
  static const OpenCl api = [] {
    const auto lib = DynamicLibrary::open({
#if defined(HWINFO_WINDOWS)
        "OpenCL.dll",
#elif defined(HWINFO_APPLE)
        "/System/Library/Frameworks/OpenCL.framework/OpenCL",
#else
        "libOpenCL.so.1",
        "libOpenCL.so",
#endif
    });
    OpenCl result;
    if (lib.load(result.get_platform_ids, "clGetPlatformIDs") &&
        lib.load(result.get_platform_info, "clGetPlatformInfo") && lib.load(result.get_device_ids, "clGetDeviceIDs") &&
        lib.load(result.get_device_info, "clGetDeviceInfo")) {
      return result;
    }
    return OpenCl{};
  }();
  return api;
}

class Device {
 public:
  Device(const OpenCl& api, cl_device_id id) : _api(api), _id(id) {}

  template <typename T>
  [[nodiscard]] std::optional<T> get(cl_device_info param) const {
    T value{};
    std::size_t size = 0;
    if (_api.get_device_info(_id, param, sizeof(T), &value, &size) != CL_SUCCESS || size != sizeof(T)) {
      return std::nullopt;
    }
    return value;
  }

  [[nodiscard]] std::optional<std::string> string(cl_device_info param) const {
    std::size_t size = 0;
    if (_api.get_device_info(_id, param, 0, nullptr, &size) != CL_SUCCESS || size == 0) {
      return std::nullopt;
    }
    std::string value(size, '\0');
    if (_api.get_device_info(_id, param, size, value.data(), nullptr) != CL_SUCCESS) {
      return std::nullopt;
    }
    value.resize(std::strlen(value.c_str()));
    return std::string(trim(value));
  }

 private:
  const OpenCl& _api;
  cl_device_id _id;
};

bool has_extension(std::string_view extensions, std::string_view name) {
  for (const auto word : words(extensions)) {
    if (word == name) {
      return true;
    }
  }
  return false;
}

std::optional<std::string> pci_address(const Device& device, std::string_view extensions, std::uint32_t vendor_id) {
  if (has_extension(extensions, "cl_khr_pci_bus_info")) {
    if (const auto bus = device.get<cl_device_pci_bus_info_khr>(CL_DEVICE_PCI_BUS_INFO_KHR)) {
      return format_pci_address(bus->pci_domain, bus->pci_bus, bus->pci_device, bus->pci_function);
    }
  }
  if (vendor_id == pci_vendor::nvidia && has_extension(extensions, "cl_nv_device_attribute_query")) {
    const auto bus = device.get<cl_uint>(CL_DEVICE_PCI_BUS_ID_NV);
    const auto slot = device.get<cl_uint>(CL_DEVICE_PCI_SLOT_ID_NV);
    if (bus && slot) {
      const auto domain = device.get<cl_uint>(CL_DEVICE_PCI_DOMAIN_ID_NV).value_or(0);
      return format_pci_address(domain, *bus, *slot >> 3, *slot & 0x7);
    }
  }
  if (has_extension(extensions, "cl_amd_device_attribute_query")) {
    if (const auto topology = device.get<cl_device_topology_amd>(CL_DEVICE_TOPOLOGY_AMD);
        topology && topology->raw.type == CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD) {
      return format_pci_address(0, static_cast<std::uint8_t>(topology->pcie.bus),
                                static_cast<std::uint8_t>(topology->pcie.device),
                                static_cast<std::uint8_t>(topology->pcie.function));
    }
  }
  return std::nullopt;
}

void read_identity(const Device& device, std::string_view extensions, Gpu& gpu) {
  if (!has_extension(extensions, "cl_khr_device_uuid")) {
    return;
  }
  if (const auto uuid = device.get<std::array<std::uint8_t, CL_UUID_SIZE_KHR>>(CL_DEVICE_UUID_KHR)) {
    gpu.uuid = GpuUuid{*uuid};
  }
  if (device.get<cl_bool>(CL_DEVICE_LUID_VALID_KHR).value_or(0) != 0) {
    if (const auto luid = device.get<std::array<std::uint8_t, CL_LUID_SIZE_KHR>>(CL_DEVICE_LUID_KHR)) {
      // a Windows LUID: {DWORD LowPart; LONG HighPart}, little endian
      std::uint64_t value = 0;
      for (std::size_t i = 0; i < luid->size(); ++i) {
        value |= std::uint64_t{(*luid)[i]} << (8 * i);
      }
      gpu.luid = value;
    }
  }
}

void read_vendor_specific(const Device& device, std::string_view extensions, std::uint32_t vendor_id, Gpu& gpu) {
  if (vendor_id == pci_vendor::nvidia && has_extension(extensions, "cl_nv_device_attribute_query")) {
    const auto major = device.get<cl_uint>(CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV);
    const auto minor = device.get<cl_uint>(CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV);
    if (major && minor) {
      apply_compute_capability(gpu, static_cast<int>(*major), static_cast<int>(*minor));
    }
    if (const auto integrated = device.get<cl_bool>(CL_DEVICE_INTEGRATED_MEMORY_NV)) {
      gpu.type = *integrated != 0 ? GpuType::integrated : GpuType::discrete;
    }
  }
  if (vendor_id == pci_vendor::amd && has_extension(extensions, "cl_amd_device_attribute_query")) {
    // ROCm / AMD APP report the gfx target as the device name and the marketing name as the board name
    if (const auto name = device.string(CL_DEVICE_NAME); name && name->starts_with("gfx")) {
      gpu.compute_capability = name->substr(0, name->find(':'));  // "gfx90a:sramecc+:xnack-"
    }
    if (auto board = device.string(CL_DEVICE_BOARD_NAME_AMD); board && !board->empty()) {
      gpu.name = std::move(*board);
    }
    if (gpu.compute_units) {
      gpu.cores = *gpu.compute_units * 64;  // GCN: 4 x SIMD16, RDNA: 2 x SIMD32 per compute unit
    }
  }
  if (vendor_id == pci_vendor::intel && has_extension(extensions, "cl_intel_device_attribute_query")) {
    if (const auto ip = device.get<cl_uint>(CL_DEVICE_IP_VERSION_INTEL); ip && *ip != 0) {
      apply_intel_ip_version(gpu, *ip);
    }
    const auto slices = device.get<cl_uint>(CL_DEVICE_NUM_SLICES_INTEL);
    const auto subslices = device.get<cl_uint>(CL_DEVICE_NUM_SUB_SLICES_PER_SLICE_INTEL);
    if (slices && subslices) {
      gpu.compute_units = *slices * *subslices;  // Xe-cores; CL_DEVICE_MAX_COMPUTE_UNITS counts EUs on Intel
    }
  }
}

Gpu read_device(const Device& device) {
  Gpu gpu;
  const auto extensions = device.string(CL_DEVICE_EXTENSIONS).value_or("");
  const auto vendor_id = device.get<cl_uint>(CL_DEVICE_VENDOR_ID).value_or(0);
  if (vendor_id != 0 && vendor_id <= 0xffff) {
    gpu.pci = PciDevice{.vendor_id = static_cast<std::uint16_t>(vendor_id),
                        .address = pci_address(device, extensions, vendor_id)};
  }
  read_identity(device, extensions, gpu);

  if (const auto units = device.get<cl_uint>(CL_DEVICE_MAX_COMPUTE_UNITS); units && *units > 0) {
    gpu.compute_units = *units;
  }
  if (const auto mhz = device.get<cl_uint>(CL_DEVICE_MAX_CLOCK_FREQUENCY); mhz && *mhz > 0) {
    gpu.max_frequency = std::uint64_t{*mhz} * FrequencyUnit::MHz;
  }
  if (const auto cache = device.get<cl_ulong>(CL_DEVICE_GLOBAL_MEM_CACHE_SIZE); cache && *cache > 0) {
    gpu.l2_cache = Bytes{*cache};
  }
  const auto unified = device.get<cl_bool>(CL_DEVICE_HOST_UNIFIED_MEMORY);
  if (unified) {
    gpu.type = *unified != 0 ? GpuType::integrated : GpuType::discrete;
  }
  // for integrated GPUs, the global memory is the part of the system RAM the driver lets the GPU use
  if (const auto memory = device.get<cl_ulong>(CL_DEVICE_GLOBAL_MEM_SIZE); memory && unified == cl_bool{0}) {
    gpu.dedicated_memory = Bytes{*memory};
  }
  gpu.driver_version = device.string(CL_DRIVER_VERSION);
  if (auto version = device.string(CL_DEVICE_VERSION); version && version->starts_with("OpenCL ")) {
    gpu.compute_apis.push_back(GpuApi{.name = "OpenCL", .version = version->substr(7)});
  }
  read_vendor_specific(device, extensions, vendor_id, gpu);
  return gpu;
}

}  // namespace

BackendResult opencl_devices() {
  const auto& api = opencl();
  if (!api) {
    return {};
  }
  cl_uint platform_count = 0;
  if (api.get_platform_ids(0, nullptr, &platform_count) != CL_SUCCESS || platform_count == 0) {
    return {};
  }
  std::vector<cl_platform_id> platforms(platform_count);
  if (api.get_platform_ids(platform_count, platforms.data(), nullptr) != CL_SUCCESS) {
    return {};
  }
  BackendResult result;
  for (const auto platform : platforms) {
    cl_uint device_count = 0;
    if (api.get_device_ids(platform, CL_DEVICE_TYPE_GPU, 0, nullptr, &device_count) != CL_SUCCESS) {
      continue;  // CL_DEVICE_NOT_FOUND: a CPU-only platform
    }
    std::vector<cl_device_id> devices(device_count);
    if (api.get_device_ids(platform, CL_DEVICE_TYPE_GPU, device_count, devices.data(), nullptr) != CL_SUCCESS) {
      continue;
    }
    for (const auto id : devices) {
      result.push_back(read_device(Device(api, id)));
    }
  }
  return result;
}

}  // namespace hwinfo::internal::gpu
