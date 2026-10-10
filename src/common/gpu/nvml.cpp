// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// NVML backend: NVIDIA's management library, shipped with every NVIDIA driver.
// Primary source for NVIDIA GPUs.

#include "internal/gpu_api/nvml.h"

#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>

#include "internal/dynamic_library.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/gpu_monitor.h"
#include "internal/pci.h"

namespace hwinfo::internal {

namespace nvml {

const Api* api() {
  static const std::optional<Api> loaded = []() -> std::optional<Api> {
    const auto lib = DynamicLibrary::open({
#ifdef HWINFO_WINDOWS
        "nvml.dll",
#else
        "libnvidia-ml.so.1",
        "libnvidia-ml.so",
#endif
    });
    nvmlInit_v2_t init = nullptr;
    if (!lib.load(init, "nvmlInit_v2") || init() != NVML_SUCCESS) {
      return std::nullopt;
    }
    // never shut down: the library stays loaded for the lifetime of the process
    Api api;
    if (!lib.load(api.device_get_count, "nvmlDeviceGetCount_v2") ||
        !lib.load(api.device_get_handle_by_index, "nvmlDeviceGetHandleByIndex_v2")) {
      return std::nullopt;
    }
    lib.load(api.system_get_driver_version, "nvmlSystemGetDriverVersion");
    lib.load(api.system_get_cuda_driver_version, "nvmlSystemGetCudaDriverVersion_v2");
    lib.load(api.device_get_handle_by_pci_bus_id, "nvmlDeviceGetHandleByPciBusId_v2");
    lib.load(api.device_get_name, "nvmlDeviceGetName");
    lib.load(api.device_get_uuid, "nvmlDeviceGetUUID");
    lib.load(api.device_get_serial, "nvmlDeviceGetSerial");
    lib.load(api.device_get_vbios_version, "nvmlDeviceGetVbiosVersion");
    lib.load(api.device_get_pci_info, "nvmlDeviceGetPciInfo_v3");
    lib.load(api.device_get_memory_info, "nvmlDeviceGetMemoryInfo");
    lib.load(api.device_get_max_clock_info, "nvmlDeviceGetMaxClockInfo");
    lib.load(api.device_get_clock_info, "nvmlDeviceGetClockInfo");
    lib.load(api.device_get_cuda_compute_capability, "nvmlDeviceGetCudaComputeCapability");
    lib.load(api.device_get_architecture, "nvmlDeviceGetArchitecture");
    lib.load(api.device_get_num_gpu_cores, "nvmlDeviceGetNumGpuCores");
    lib.load(api.device_get_memory_bus_width, "nvmlDeviceGetMemoryBusWidth");
    lib.load(api.device_get_power_management_default_limit, "nvmlDeviceGetPowerManagementDefaultLimit");
    lib.load(api.device_get_power_usage, "nvmlDeviceGetPowerUsage");
    lib.load(api.device_get_max_pcie_link_generation, "nvmlDeviceGetMaxPcieLinkGeneration");
    lib.load(api.device_get_max_pcie_link_width, "nvmlDeviceGetMaxPcieLinkWidth");
    lib.load(api.device_get_curr_pcie_link_generation, "nvmlDeviceGetCurrPcieLinkGeneration");
    lib.load(api.device_get_curr_pcie_link_width, "nvmlDeviceGetCurrPcieLinkWidth");
    lib.load(api.device_get_fan_speed, "nvmlDeviceGetFanSpeed");
    lib.load(api.device_get_utilization_rates, "nvmlDeviceGetUtilizationRates");
    lib.load(api.device_get_encoder_utilization, "nvmlDeviceGetEncoderUtilization");
    lib.load(api.device_get_decoder_utilization, "nvmlDeviceGetDecoderUtilization");
    lib.load(api.device_get_temperature, "nvmlDeviceGetTemperature");
    return api;
  }();
  return loaded ? &*loaded : nullptr;
}

}  // namespace nvml

namespace gpu {

namespace {

using namespace internal::nvml;

template <std::size_t Size>
std::optional<std::string> string(nvmlDeviceGetString_t fn, nvmlDevice_t device) {
  std::array<char, Size> buffer{};
  if (fn == nullptr || fn(device, buffer.data(), static_cast<unsigned int>(buffer.size())) != NVML_SUCCESS) {
    return std::nullopt;
  }
  std::string value(buffer.data());
  return value.empty() ? std::nullopt : std::optional(value);
}

std::optional<unsigned int> uint(nvmlDeviceGetUInt_t fn, nvmlDevice_t device) {
  unsigned int value = 0;
  if (fn == nullptr || fn(device, &value) != NVML_SUCCESS) {
    return std::nullopt;
  }
  return value;
}

std::optional<Hertz> max_clock(const Api& api, nvmlDevice_t device, nvmlClockType_t type) {
  unsigned int mhz = 0;
  if (api.device_get_max_clock_info == nullptr || api.device_get_max_clock_info(device, type, &mhz) != NVML_SUCCESS ||
      mhz == 0) {
    return std::nullopt;
  }
  return std::uint64_t{mhz} * FrequencyUnit::MHz;
}

std::optional<std::string> architecture(nvmlDeviceArchitecture_t arch) {
  switch (arch) {
    case NVML_DEVICE_ARCH_KEPLER:
      return "Kepler";
    case NVML_DEVICE_ARCH_MAXWELL:
      return "Maxwell";
    case NVML_DEVICE_ARCH_PASCAL:
      return "Pascal";
    case NVML_DEVICE_ARCH_VOLTA:
      return "Volta";
    case NVML_DEVICE_ARCH_TURING:
      return "Turing";
    case NVML_DEVICE_ARCH_AMPERE:
      return "Ampere";
    case NVML_DEVICE_ARCH_ADA:
      return "Ada Lovelace";
    case NVML_DEVICE_ARCH_HOPPER:
      return "Hopper";
    case NVML_DEVICE_ARCH_BLACKWELL:
      return "Blackwell";
    default:
      return std::nullopt;
  }
}

Gpu read_device(const Api& api, nvmlDevice_t device, const std::optional<std::string>& driver_version,
                const std::optional<GpuApi>& cuda) {
  Gpu gpu;
  gpu.name = string<NVML_DEVICE_NAME_V2_BUFFER_SIZE>(api.device_get_name, device).value_or("");
  if (const auto uuid = string<NVML_DEVICE_UUID_V2_BUFFER_SIZE>(api.device_get_uuid, device)) {
    gpu.uuid = parse_uuid(*uuid);
  }
  if (nvmlPciInfo_t pci{}; api.device_get_pci_info && api.device_get_pci_info(device, &pci) == NVML_SUCCESS) {
    gpu.pci = PciDevice{
        .vendor_id = static_cast<std::uint16_t>(pci.pciDeviceId & 0xffff),
        .device_id = static_cast<std::uint16_t>(pci.pciDeviceId >> 16),
        .address = normalize_pci_address(pci.busId),
    };
    const auto generation = uint(api.device_get_max_pcie_link_generation, device);
    const auto width = uint(api.device_get_max_pcie_link_width, device);
    if (generation && width && *generation > 0 && *width > 0) {
      gpu.pci->max_link = PcieLink{.generation = *generation, .width = *width};
    }
  }

  gpu.cores = uint(api.device_get_num_gpu_cores, device);
  int major = 0;
  int minor = 0;
  if (api.device_get_cuda_compute_capability &&
      api.device_get_cuda_compute_capability(device, &major, &minor) == NVML_SUCCESS) {
    apply_compute_capability(gpu, major, minor);
  }
  if (nvmlDeviceArchitecture_t arch = 0;
      api.device_get_architecture && api.device_get_architecture(device, &arch) == NVML_SUCCESS) {
    if (auto name = architecture(arch)) {
      gpu.architecture = std::move(name);
    }
  }

  gpu.driver_version = driver_version;
  gpu.vbios_version = string<NVML_DEVICE_VBIOS_VERSION_BUFFER_SIZE>(api.device_get_vbios_version, device);
  gpu.serial = string<NVML_DEVICE_SERIAL_BUFFER_SIZE>(api.device_get_serial, device);
  if (nvmlMemory_t memory{};
      api.device_get_memory_info && api.device_get_memory_info(device, &memory) == NVML_SUCCESS) {
    gpu.dedicated_memory = Bytes{memory.total};
  }
  gpu.memory_bus_width = uint(api.device_get_memory_bus_width, device);
  gpu.max_frequency = max_clock(api, device, NVML_CLOCK_GRAPHICS);
  gpu.max_memory_frequency = max_clock(api, device, NVML_CLOCK_MEM);
  if (const auto mw = uint(api.device_get_power_management_default_limit, device); mw && *mw > 0) {
    gpu.power_limit = std::uint64_t{*mw} * PowerUnit::mW;
  }
  if (cuda) {
    gpu.compute_apis.push_back(*cuda);
  }
  return gpu;
}

}  // namespace

BackendResult nvml_devices() {
  const Api* api = nvml::api();
  unsigned int count = 0;
  if (api == nullptr || api->device_get_count(&count) != NVML_SUCCESS) {
    return {};
  }
  std::optional<std::string> driver_version;
  if (std::array<char, NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE> buffer{};
      api->system_get_driver_version &&
      api->system_get_driver_version(buffer.data(), static_cast<unsigned int>(buffer.size())) == NVML_SUCCESS) {
    driver_version = std::string(buffer.data());
  }
  std::optional<GpuApi> cuda;
  if (int version = 0; api->system_get_cuda_driver_version &&
                       api->system_get_cuda_driver_version(&version) == NVML_SUCCESS && version > 0) {
    cuda = GpuApi{.name = "CUDA", .version = cuda_version(version)};
  }

  BackendResult result;
  for (unsigned int i = 0; i < count; ++i) {
    nvmlDevice_t device = nullptr;
    if (api->device_get_handle_by_index(i, &device) == NVML_SUCCESS) {
      result.push_back(read_device(*api, device, driver_version, cuda));
    }
  }
  return result;
}

}  // namespace gpu

}  // namespace hwinfo::internal

namespace hwinfo::internal::gpu {

namespace {

class NvmlSource final : public StatusSource {
 public:
  NvmlSource(const nvml::Api& api, nvml::nvmlDevice_t device) : _api(api), _device(device) {}

  void sample(GpuStatus& status) override {
    using namespace internal::nvml;
    if (nvmlUtilization_t rates{};
        _api.device_get_utilization_rates && _api.device_get_utilization_rates(_device, &rates) == NVML_SUCCESS) {
      fill(status.utilization, std::optional(rates.gpu / 100.0));
      fill(status.memory_utilization, std::optional(rates.memory / 100.0));
    }
    const auto encoder = codec(_api.device_get_encoder_utilization);
    const auto decoder = codec(_api.device_get_decoder_utilization);
    if (encoder || decoder) {
      fill(status.video_utilization, std::optional(std::max(encoder.value_or(0), decoder.value_or(0)) / 100.0));
    }
    if (nvmlMemory_t memory{};
        _api.device_get_memory_info && _api.device_get_memory_info(_device, &memory) == NVML_SUCCESS) {
      fill(status.memory_used, std::optional(Bytes{memory.used}));
      fill(status.memory_total, std::optional(Bytes{memory.total}));
    }
    if (unsigned int celsius = 0;
        _api.device_get_temperature &&
        _api.device_get_temperature(_device, NVML_TEMPERATURE_GPU, &celsius) == NVML_SUCCESS) {
      fill(status.temperature, std::optional(static_cast<double>(celsius)));
    }
    if (const auto mw = uint(_api.device_get_power_usage, _device)) {
      fill(status.power, std::optional(std::uint64_t{*mw} * PowerUnit::mW));
    }
    fill(status.frequency, clock(NVML_CLOCK_GRAPHICS));
    fill(status.memory_frequency, clock(NVML_CLOCK_MEM));
    if (const auto percent = uint(_api.device_get_fan_speed, _device)) {
      fill(status.fan_speed, std::optional(*percent / 100.0));
    }
    const auto generation = uint(_api.device_get_curr_pcie_link_generation, _device);
    const auto width = uint(_api.device_get_curr_pcie_link_width, _device);
    if (generation && width && *generation > 0 && *width > 0) {
      fill(status.pcie_link, std::optional(PcieLink{.generation = *generation, .width = *width}));
    }
  }

 private:
  std::optional<unsigned int> codec(nvml::nvmlDeviceGetCodecUtilization_t fn) const {
    unsigned int percent = 0;
    unsigned int period = 0;
    if (fn == nullptr || fn(_device, &percent, &period) != nvml::NVML_SUCCESS) {
      return std::nullopt;
    }
    return percent;
  }

  std::optional<Hertz> clock(nvml::nvmlClockType_t type) const {
    unsigned int mhz = 0;
    if (_api.device_get_clock_info == nullptr ||
        _api.device_get_clock_info(_device, type, &mhz) != nvml::NVML_SUCCESS) {
      return std::nullopt;
    }
    return std::uint64_t{mhz} * FrequencyUnit::MHz;
  }

  const nvml::Api& _api;
  nvml::nvmlDevice_t _device;
};

}  // namespace

std::unique_ptr<StatusSource> nvml_source(const Gpu& gpu) {
  // check the vendor first: initializing NVML wakes up every NVIDIA GPU
  if (!gpu.pci || gpu.pci->vendor_id != pci_vendor::nvidia) {
    return nullptr;
  }
  const nvml::Api* api = nvml::api();
  if (api == nullptr) {
    return nullptr;
  }
  // find the device by PCI address, or by UUID
  unsigned int count = 0;
  if (api->device_get_count(&count) != nvml::NVML_SUCCESS) {
    return nullptr;
  }
  for (unsigned int i = 0; i < count; ++i) {
    nvml::nvmlDevice_t device = nullptr;
    if (api->device_get_handle_by_index(i, &device) != nvml::NVML_SUCCESS) {
      continue;
    }
    const Gpu identity = read_device(*api, device, std::nullopt, std::nullopt);
    const bool same_address = gpu.pci->address && identity.pci && identity.pci->address == gpu.pci->address;
    const bool same_uuid = gpu.uuid && identity.uuid == gpu.uuid;
    if (same_address || same_uuid) {
      return std::make_unique<NvmlSource>(*api, device);
    }
  }
  return nullptr;
}

}  // namespace hwinfo::internal::gpu
