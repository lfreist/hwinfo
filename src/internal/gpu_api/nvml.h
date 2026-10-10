// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// The subset of the NVIDIA Management Library (NVML) used by hwinfo, for loading libnvidia-ml / nvml.dll at runtime.
// Mirrors nvml.h of NVML API version 12 (CUDA 12.x drivers). Functions newer than the oldest supported driver are
// optional and looked up individually. NVML uses the default calling convention on all platforms.
// Not part of the public API.

#pragma once

#include <cstddef>

namespace hwinfo::internal::nvml {

using nvmlReturn_t = int;
using nvmlDevice_t = struct nvmlDevice_st*;
using nvmlClockType_t = int;
using nvmlTemperatureSensors_t = int;
using nvmlDeviceArchitecture_t = unsigned int;

constexpr nvmlReturn_t NVML_SUCCESS = 0;

constexpr nvmlClockType_t NVML_CLOCK_GRAPHICS = 0;
constexpr nvmlClockType_t NVML_CLOCK_SM = 1;
constexpr nvmlClockType_t NVML_CLOCK_MEM = 2;

constexpr nvmlTemperatureSensors_t NVML_TEMPERATURE_GPU = 0;

constexpr std::size_t NVML_DEVICE_PCI_BUS_ID_BUFFER_SIZE = 32;
constexpr std::size_t NVML_DEVICE_PCI_BUS_ID_BUFFER_V2_SIZE = 16;
constexpr std::size_t NVML_DEVICE_UUID_V2_BUFFER_SIZE = 96;
constexpr std::size_t NVML_DEVICE_NAME_V2_BUFFER_SIZE = 96;
constexpr std::size_t NVML_DEVICE_SERIAL_BUFFER_SIZE = 30;
constexpr std::size_t NVML_DEVICE_VBIOS_VERSION_BUFFER_SIZE = 32;
constexpr std::size_t NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE = 80;

struct nvmlPciInfo_t {
  char busIdLegacy[NVML_DEVICE_PCI_BUS_ID_BUFFER_V2_SIZE];
  unsigned int domain;
  unsigned int bus;
  unsigned int device;
  unsigned int pciDeviceId;  // device id << 16 | vendor id
  unsigned int pciSubSystemId;
  char busId[NVML_DEVICE_PCI_BUS_ID_BUFFER_SIZE];  // "00000000:01:00.0"
};

struct nvmlMemory_t {
  unsigned long long total;
  unsigned long long free;
  unsigned long long used;
};

struct nvmlUtilization_t {
  unsigned int gpu;     // percent of time a kernel / graphics work was executing
  unsigned int memory;  // percent of time device memory was read or written
};

// nvmlDeviceGetArchitecture
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_KEPLER = 2;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_MAXWELL = 3;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_PASCAL = 4;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_VOLTA = 5;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_TURING = 6;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_AMPERE = 7;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_ADA = 8;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_HOPPER = 9;
constexpr nvmlDeviceArchitecture_t NVML_DEVICE_ARCH_BLACKWELL = 10;

using nvmlInit_v2_t = nvmlReturn_t (*)();
using nvmlSystemGetDriverVersion_t = nvmlReturn_t (*)(char* version, unsigned int length);
using nvmlSystemGetCudaDriverVersion_v2_t = nvmlReturn_t (*)(int* version);
using nvmlDeviceGetCount_v2_t = nvmlReturn_t (*)(unsigned int* count);
using nvmlDeviceGetHandleByIndex_v2_t = nvmlReturn_t (*)(unsigned int index, nvmlDevice_t* device);
using nvmlDeviceGetHandleByPciBusId_v2_t = nvmlReturn_t (*)(const char* bus_id, nvmlDevice_t* device);
using nvmlDeviceGetString_t = nvmlReturn_t (*)(nvmlDevice_t device, char* value, unsigned int length);
using nvmlDeviceGetUInt_t = nvmlReturn_t (*)(nvmlDevice_t device, unsigned int* value);
using nvmlDeviceGetPciInfo_v3_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlPciInfo_t* pci);
using nvmlDeviceGetMemoryInfo_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlMemory_t* memory);
using nvmlDeviceGetClock_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlClockType_t type, unsigned int* mhz);
using nvmlDeviceGetCudaComputeCapability_t = nvmlReturn_t (*)(nvmlDevice_t device, int* major, int* minor);
using nvmlDeviceGetArchitecture_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlDeviceArchitecture_t* arch);
using nvmlDeviceGetUtilizationRates_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlUtilization_t* utilization);
using nvmlDeviceGetCodecUtilization_t = nvmlReturn_t (*)(nvmlDevice_t device, unsigned int* utilization,
                                                         unsigned int* sampling_period_us);
using nvmlDeviceGetTemperature_t = nvmlReturn_t (*)(nvmlDevice_t device, nvmlTemperatureSensors_t sensor,
                                                    unsigned int* celsius);

static_assert(sizeof(nvmlPciInfo_t) == 68);
static_assert(sizeof(nvmlMemory_t) == 24);

}  // namespace hwinfo::internal::nvml

namespace hwinfo::internal::nvml {

// The functions of an initialized NVML. Optional functions (added in later drivers) may be nullptr.
struct Api {
  nvmlSystemGetDriverVersion_t system_get_driver_version = nullptr;
  nvmlSystemGetCudaDriverVersion_v2_t system_get_cuda_driver_version = nullptr;
  nvmlDeviceGetCount_v2_t device_get_count = nullptr;
  nvmlDeviceGetHandleByIndex_v2_t device_get_handle_by_index = nullptr;
  nvmlDeviceGetHandleByPciBusId_v2_t device_get_handle_by_pci_bus_id = nullptr;
  nvmlDeviceGetString_t device_get_name = nullptr;
  nvmlDeviceGetString_t device_get_uuid = nullptr;
  nvmlDeviceGetString_t device_get_serial = nullptr;
  nvmlDeviceGetString_t device_get_vbios_version = nullptr;
  nvmlDeviceGetPciInfo_v3_t device_get_pci_info = nullptr;
  nvmlDeviceGetMemoryInfo_t device_get_memory_info = nullptr;
  nvmlDeviceGetClock_t device_get_max_clock_info = nullptr;
  nvmlDeviceGetClock_t device_get_clock_info = nullptr;
  nvmlDeviceGetCudaComputeCapability_t device_get_cuda_compute_capability = nullptr;
  nvmlDeviceGetArchitecture_t device_get_architecture = nullptr;
  nvmlDeviceGetUInt_t device_get_num_gpu_cores = nullptr;
  nvmlDeviceGetUInt_t device_get_memory_bus_width = nullptr;
  nvmlDeviceGetUInt_t device_get_power_management_default_limit = nullptr;  // mW
  nvmlDeviceGetUInt_t device_get_power_usage = nullptr;                     // mW
  nvmlDeviceGetUInt_t device_get_max_pcie_link_generation = nullptr;
  nvmlDeviceGetUInt_t device_get_max_pcie_link_width = nullptr;
  nvmlDeviceGetUInt_t device_get_curr_pcie_link_generation = nullptr;
  nvmlDeviceGetUInt_t device_get_curr_pcie_link_width = nullptr;
  nvmlDeviceGetUInt_t device_get_fan_speed = nullptr;  // percent
  nvmlDeviceGetUtilizationRates_t device_get_utilization_rates = nullptr;
  nvmlDeviceGetCodecUtilization_t device_get_encoder_utilization = nullptr;
  nvmlDeviceGetCodecUtilization_t device_get_decoder_utilization = nullptr;
  nvmlDeviceGetTemperature_t device_get_temperature = nullptr;
};

// The process wide NVML, loaded and initialized on first use; nullptr if NVML is not available.
const Api* api();

}  // namespace hwinfo::internal::nvml
