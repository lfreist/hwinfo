// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// The subset of oneAPI Level Zero (core and Sysman) used by hwinfo, for loading ze_loader at runtime.
// Mirrors level_zero/ze_api.h and zes_api.h of Level Zero 1.x; the structures carry a `stype` and are ABI stable
// within major version 1.
// Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#include <cstddef>
#include <cstdint>

#ifdef HWINFO_WINDOWS
#define HWINFO_ZE_CALL __cdecl
#else
#define HWINFO_ZE_CALL
#endif

namespace hwinfo::internal::ze {

using ze_result_t = std::int32_t;
using ze_bool_t = std::uint8_t;
using ze_structure_type_t = std::int32_t;
using zes_structure_type_t = std::int32_t;
using ze_driver_handle_t = struct _ze_driver_handle_t*;
using ze_device_handle_t = struct _ze_device_handle_t*;
using zes_driver_handle_t = struct _zes_driver_handle_t*;
using zes_device_handle_t = struct _zes_device_handle_t*;
using zes_temp_handle_t = struct _zes_temp_handle_t*;
using zes_pwr_handle_t = struct _zes_pwr_handle_t*;
using zes_engine_handle_t = struct _zes_engine_handle_t*;
using zes_mem_handle_t = struct _zes_mem_handle_t*;
using zes_freq_handle_t = struct _zes_freq_handle_t*;

constexpr ze_result_t ZE_RESULT_SUCCESS = 0;
constexpr std::uint32_t ZE_INIT_FLAG_GPU_ONLY = 1;
constexpr std::int32_t ZE_DEVICE_TYPE_GPU = 1;
constexpr std::uint32_t ZE_DEVICE_PROPERTY_FLAG_INTEGRATED = 1;
constexpr std::uint32_t ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE = 2;
constexpr std::size_t ZE_MAX_DEVICE_NAME = 256;
constexpr std::size_t ZE_MAX_DEVICE_UUID_SIZE = 16;
constexpr std::size_t ZES_STRING_PROPERTY_SIZE = 64;

constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES = 0x3;
constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES = 0x7;
constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES = 0x9;
constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES = 0x10008;
constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES = 0x1000d;
constexpr ze_structure_type_t ZE_STRUCTURE_TYPE_DEVICE_IP_VERSION_EXT = 0x1000f;

constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_PCI_PROPERTIES = 0x2;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES = 0x5;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_FREQ_PROPERTIES = 0x9;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_MEM_PROPERTIES = 0xb;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_TEMP_PROPERTIES = 0x14;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_PCI_STATE = 0x17;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_FREQ_STATE = 0x1b;
constexpr zes_structure_type_t ZES_STRUCTURE_TYPE_MEM_STATE = 0x1e;

constexpr std::int32_t ZES_ENGINE_GROUP_ALL = 0;
constexpr std::int32_t ZES_ENGINE_GROUP_COMPUTE_ALL = 1;
constexpr std::int32_t ZES_ENGINE_GROUP_MEDIA_ALL = 2;
constexpr std::int32_t ZES_ENGINE_GROUP_RENDER_ALL = 12;
constexpr std::int32_t ZES_FREQ_DOMAIN_GPU = 0;
constexpr std::int32_t ZES_FREQ_DOMAIN_MEMORY = 1;
constexpr std::int32_t ZES_MEM_LOC_DEVICE = 1;
constexpr std::int32_t ZES_TEMP_SENSORS_GLOBAL = 0;
constexpr std::int32_t ZES_TEMP_SENSORS_GPU = 1;

// ze_api.h

struct ze_device_uuid_t {
  std::uint8_t id[ZE_MAX_DEVICE_UUID_SIZE];
};

struct ze_device_properties_t {
  ze_structure_type_t stype;
  void* pNext;
  std::int32_t type;  // ze_device_type_t
  std::uint32_t vendorId;
  std::uint32_t deviceId;
  std::uint32_t flags;  // ze_device_property_flags_t
  std::uint32_t subdeviceId;
  std::uint32_t coreClockRate;  // MHz
  std::uint64_t maxMemAllocSize;
  std::uint32_t maxHardwareContexts;
  std::uint32_t maxCommandQueuePriority;
  std::uint32_t numThreadsPerEU;
  std::uint32_t physicalEUSimdWidth;
  std::uint32_t numEUsPerSubslice;
  std::uint32_t numSubslicesPerSlice;
  std::uint32_t numSlices;
  std::uint64_t timerResolution;
  std::uint32_t timestampValidBits;
  std::uint32_t kernelTimestampValidBits;
  ze_device_uuid_t uuid;
  char name[ZE_MAX_DEVICE_NAME];
};

struct ze_device_memory_properties_t {
  ze_structure_type_t stype;
  void* pNext;
  std::uint32_t flags;
  std::uint32_t maxClockRate;  // MHz
  std::uint32_t maxBusWidth;   // bits
  std::uint64_t totalSize;
  char name[ZE_MAX_DEVICE_NAME];
};

struct ze_device_cache_properties_t {
  ze_structure_type_t stype;
  void* pNext;
  std::uint32_t flags;
  std::size_t cacheSize;
};

struct ze_pci_address_ext_t {
  std::uint32_t domain;
  std::uint32_t bus;
  std::uint32_t device;
  std::uint32_t function;
};

struct ze_pci_speed_ext_t {
  std::int32_t genVersion;  // -1 if unknown
  std::int32_t width;       // -1 if unknown
  std::int64_t maxBandwidth;
};

struct ze_pci_ext_properties_t {
  ze_structure_type_t stype;
  void* pNext;
  ze_pci_address_ext_t address;
  ze_pci_speed_ext_t maxSpeed;
};

struct ze_device_luid_ext_properties_t {
  ze_structure_type_t stype;
  void* pNext;
  std::uint8_t luid[8];
  std::uint32_t nodeMask;
};

struct ze_device_ip_version_ext_t {
  ze_structure_type_t stype;
  const void* pNext;
  std::uint32_t ipVersion;  // major << 22 | minor << 14 | revision
};

using zeInit_t = ze_result_t(HWINFO_ZE_CALL*)(std::uint32_t flags);
using zeDriverGet_t = ze_result_t(HWINFO_ZE_CALL*)(std::uint32_t* count, ze_driver_handle_t* drivers);
using zeDriverGetApiVersion_t = ze_result_t(HWINFO_ZE_CALL*)(ze_driver_handle_t driver, std::uint32_t* version);
using zeDeviceGet_t = ze_result_t(HWINFO_ZE_CALL*)(ze_driver_handle_t driver, std::uint32_t* count,
                                                   ze_device_handle_t* devices);
using zeDeviceGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(ze_device_handle_t device,
                                                             ze_device_properties_t* properties);
using zeDeviceGetMemoryProperties_t = ze_result_t(HWINFO_ZE_CALL*)(ze_device_handle_t device, std::uint32_t* count,
                                                                   ze_device_memory_properties_t* properties);
using zeDeviceGetCacheProperties_t = ze_result_t(HWINFO_ZE_CALL*)(ze_device_handle_t device, std::uint32_t* count,
                                                                  ze_device_cache_properties_t* properties);
using zeDevicePciGetPropertiesExt_t = ze_result_t(HWINFO_ZE_CALL*)(ze_device_handle_t device,
                                                                   ze_pci_ext_properties_t* properties);

// zes_api.h

struct zes_pci_address_t {
  std::uint32_t domain;
  std::uint32_t bus;
  std::uint32_t device;
  std::uint32_t function;
};

struct zes_pci_speed_t {
  std::int32_t gen;    // -1 if unknown
  std::int32_t width;  // -1 if unknown
  std::int64_t maxBandwidth;
};

struct zes_pci_properties_t {
  zes_structure_type_t stype;
  void* pNext;
  zes_pci_address_t address;
  zes_pci_speed_t maxSpeed;
  ze_bool_t haveBandwidthCounters;
  ze_bool_t havePacketCounters;
  ze_bool_t haveReplayCounters;
};

struct zes_pci_state_t {
  zes_structure_type_t stype;
  const void* pNext;
  std::int32_t status;
  std::uint32_t qualityIssues;
  std::uint32_t stabilityIssues;
  zes_pci_speed_t speed;
};

struct zes_temp_properties_t {
  zes_structure_type_t stype;
  void* pNext;
  std::int32_t type;  // zes_temp_sensors_t
  ze_bool_t onSubdevice;
  std::uint32_t subdeviceId;
  double maxTemperature;
  ze_bool_t isCriticalTempSupported;
  ze_bool_t isThreshold1Supported;
  ze_bool_t isThreshold2Supported;
};

struct zes_power_energy_counter_t {
  std::uint64_t energy;     // microjoules
  std::uint64_t timestamp;  // microseconds
};

struct zes_engine_properties_t {
  zes_structure_type_t stype;
  void* pNext;
  std::int32_t type;  // zes_engine_group_t
  ze_bool_t onSubdevice;
  std::uint32_t subdeviceId;
};

struct zes_engine_stats_t {
  std::uint64_t activeTime;  // microseconds
  std::uint64_t timestamp;   // microseconds
};

struct zes_mem_properties_t {
  zes_structure_type_t stype;
  void* pNext;
  std::int32_t type;  // zes_mem_type_t
  ze_bool_t onSubdevice;
  std::uint32_t subdeviceId;
  std::int32_t location;  // zes_mem_loc_t
  std::uint64_t physicalSize;
  std::int32_t busWidth;  // -1 if unknown
  std::int32_t numChannels;
};

struct zes_mem_state_t {
  zes_structure_type_t stype;
  const void* pNext;
  std::int32_t health;
  std::uint64_t free;
  std::uint64_t size;
};

struct zes_freq_properties_t {
  zes_structure_type_t stype;
  void* pNext;
  std::int32_t type;  // zes_freq_domain_t
  ze_bool_t onSubdevice;
  std::uint32_t subdeviceId;
  ze_bool_t canControl;
  ze_bool_t isThrottleEventSupported;
  double min;  // MHz
  double max;  // MHz
};

struct zes_freq_state_t {
  zes_structure_type_t stype;
  const void* pNext;
  double currentVoltage;
  double request;
  double tdp;
  double efficient;
  double actual;  // MHz, negative if unknown
  std::uint32_t throttleReasons;
};

using zesInit_t = ze_result_t(HWINFO_ZE_CALL*)(std::uint32_t flags);
using zesDriverGet_t = ze_result_t(HWINFO_ZE_CALL*)(std::uint32_t* count, zes_driver_handle_t* drivers);
using zesDeviceGet_t = ze_result_t(HWINFO_ZE_CALL*)(zes_driver_handle_t driver, std::uint32_t* count,
                                                    zes_device_handle_t* devices);
using zesDevicePciGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(zes_device_handle_t device,
                                                                 zes_pci_properties_t* properties);
using zesDevicePciGetState_t = ze_result_t(HWINFO_ZE_CALL*)(zes_device_handle_t device, zes_pci_state_t* state);
template <typename Handle>
using zesDeviceEnum_t = ze_result_t(HWINFO_ZE_CALL*)(zes_device_handle_t device, std::uint32_t* count, Handle* handles);
using zesTemperatureGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(zes_temp_handle_t sensor,
                                                                   zes_temp_properties_t* properties);
using zesTemperatureGetState_t = ze_result_t(HWINFO_ZE_CALL*)(zes_temp_handle_t sensor, double* celsius);
using zesDeviceGetCardPowerDomain_t = ze_result_t(HWINFO_ZE_CALL*)(zes_device_handle_t device, zes_pwr_handle_t* power);
using zesPowerGetEnergyCounter_t = ze_result_t(HWINFO_ZE_CALL*)(zes_pwr_handle_t power,
                                                                zes_power_energy_counter_t* energy);
using zesEngineGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(zes_engine_handle_t engine,
                                                              zes_engine_properties_t* properties);
using zesEngineGetActivity_t = ze_result_t(HWINFO_ZE_CALL*)(zes_engine_handle_t engine, zes_engine_stats_t* stats);
using zesMemoryGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(zes_mem_handle_t memory,
                                                              zes_mem_properties_t* properties);
using zesMemoryGetState_t = ze_result_t(HWINFO_ZE_CALL*)(zes_mem_handle_t memory, zes_mem_state_t* state);
using zesFrequencyGetProperties_t = ze_result_t(HWINFO_ZE_CALL*)(zes_freq_handle_t frequency,
                                                                 zes_freq_properties_t* properties);
using zesFrequencyGetState_t = ze_result_t(HWINFO_ZE_CALL*)(zes_freq_handle_t frequency, zes_freq_state_t* state);

// Layout checks against the upstream headers (LP64 / LLP64).
static_assert(sizeof(ze_device_properties_t) == 368);
static_assert(sizeof(ze_device_memory_properties_t) == 296);
static_assert(sizeof(ze_device_cache_properties_t) == 32);
static_assert(sizeof(ze_pci_ext_properties_t) == 48);
static_assert(sizeof(ze_device_luid_ext_properties_t) == 32);
static_assert(sizeof(zes_pci_properties_t) == 56);
static_assert(sizeof(zes_temp_properties_t) == 48);
static_assert(sizeof(zes_mem_properties_t) == 48);
static_assert(sizeof(zes_mem_state_t) == 40);
static_assert(sizeof(zes_freq_properties_t) == 48);
static_assert(sizeof(zes_freq_state_t) == 64);

// A value-initialized structure with its `stype` set, as Level Zero requires for every queried structure.
template <typename T>
T with_stype(std::int32_t stype) noexcept {
  T value{};
  value.stype = stype;
  return value;
}

// The Sysman (system management) functions of an initialized Level Zero; nullptr where not supported.
struct SysmanApi {
  zesDriverGet_t driver_get = nullptr;
  zesDeviceGet_t device_get = nullptr;
  zesDevicePciGetProperties_t device_pci_get_properties = nullptr;
  zesDevicePciGetState_t device_pci_get_state = nullptr;
  zesDeviceEnum_t<zes_temp_handle_t> device_enum_temperature_sensors = nullptr;
  zesTemperatureGetProperties_t temperature_get_properties = nullptr;
  zesTemperatureGetState_t temperature_get_state = nullptr;
  zesDeviceGetCardPowerDomain_t device_get_card_power_domain = nullptr;
  zesDeviceEnum_t<zes_pwr_handle_t> device_enum_power_domains = nullptr;
  zesPowerGetEnergyCounter_t power_get_energy_counter = nullptr;
  zesDeviceEnum_t<zes_engine_handle_t> device_enum_engine_groups = nullptr;
  zesEngineGetProperties_t engine_get_properties = nullptr;
  zesEngineGetActivity_t engine_get_activity = nullptr;
  zesDeviceEnum_t<zes_mem_handle_t> device_enum_memory_modules = nullptr;
  zesMemoryGetProperties_t memory_get_properties = nullptr;
  zesMemoryGetState_t memory_get_state = nullptr;
  zesDeviceEnum_t<zes_freq_handle_t> device_enum_frequency_domains = nullptr;
  zesFrequencyGetProperties_t frequency_get_properties = nullptr;
  zesFrequencyGetState_t frequency_get_state = nullptr;
};

const SysmanApi* sysman();

}  // namespace hwinfo::internal::ze

#undef HWINFO_ZE_CALL
