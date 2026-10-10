// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Level Zero backend: Intel's native GPU API (Arc and Xe integrated GPUs) with Sysman for memory details.

#include "internal/gpu_api/level_zero.h"

#include <algorithm>
#include <array>
#include <format>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "internal/dynamic_library.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/gpu_monitor.h"
#include "internal/pci.h"

namespace hwinfo::internal {

namespace {

DynamicLibrary loader() {
  static const DynamicLibrary lib = DynamicLibrary::open({
#ifdef HWINFO_WINDOWS
      "ze_loader.dll",
#else
      "libze_loader.so.1",
      "libze_loader.so",
#endif
  });
  return lib;
}

}  // namespace

namespace ze {

const SysmanApi* sysman() {
  static const std::optional<SysmanApi> loaded = []() -> std::optional<SysmanApi> {
    const auto lib = loader();
    zesInit_t init = nullptr;
    SysmanApi api;
    if (!lib.load(init, "zesInit") || !lib.load(api.driver_get, "zesDriverGet") ||
        !lib.load(api.device_get, "zesDeviceGet") ||
        !lib.load(api.device_pci_get_properties, "zesDevicePciGetProperties") || init(0) != ZE_RESULT_SUCCESS) {
      return std::nullopt;
    }
    lib.load(api.device_pci_get_state, "zesDevicePciGetState");
    lib.load(api.device_enum_temperature_sensors, "zesDeviceEnumTemperatureSensors");
    lib.load(api.temperature_get_properties, "zesTemperatureGetProperties");
    lib.load(api.temperature_get_state, "zesTemperatureGetState");
    lib.load(api.device_get_card_power_domain, "zesDeviceGetCardPowerDomain");
    lib.load(api.device_enum_power_domains, "zesDeviceEnumPowerDomains");
    lib.load(api.power_get_energy_counter, "zesPowerGetEnergyCounter");
    lib.load(api.device_enum_engine_groups, "zesDeviceEnumEngineGroups");
    lib.load(api.engine_get_properties, "zesEngineGetProperties");
    lib.load(api.engine_get_activity, "zesEngineGetActivity");
    lib.load(api.device_enum_memory_modules, "zesDeviceEnumMemoryModules");
    lib.load(api.memory_get_properties, "zesMemoryGetProperties");
    lib.load(api.memory_get_state, "zesMemoryGetState");
    lib.load(api.device_enum_frequency_domains, "zesDeviceEnumFrequencyDomains");
    lib.load(api.frequency_get_properties, "zesFrequencyGetProperties");
    lib.load(api.frequency_get_state, "zesFrequencyGetState");
    return api;
  }();
  return loaded ? &*loaded : nullptr;
}

}  // namespace ze

namespace gpu {

namespace {

using namespace internal::ze;

struct Core {
  zeDriverGet_t driver_get = nullptr;
  zeDriverGetApiVersion_t driver_get_api_version = nullptr;
  zeDeviceGet_t device_get = nullptr;
  zeDeviceGetProperties_t device_get_properties = nullptr;
  zeDeviceGetMemoryProperties_t device_get_memory_properties = nullptr;
  zeDeviceGetCacheProperties_t device_get_cache_properties = nullptr;
  zeDevicePciGetPropertiesExt_t device_pci_get_properties_ext = nullptr;
};

const Core* core() {
  static const std::optional<Core> loaded = []() -> std::optional<Core> {
    const auto lib = loader();
    zeInit_t init = nullptr;
    Core api;
    if (!lib.load(init, "zeInit") || !lib.load(api.driver_get, "zeDriverGet") ||
        !lib.load(api.device_get, "zeDeviceGet") || !lib.load(api.device_get_properties, "zeDeviceGetProperties") ||
        init(ZE_INIT_FLAG_GPU_ONLY) != ZE_RESULT_SUCCESS) {
      return std::nullopt;
    }
    lib.load(api.driver_get_api_version, "zeDriverGetApiVersion");
    lib.load(api.device_get_memory_properties, "zeDeviceGetMemoryProperties");
    lib.load(api.device_get_cache_properties, "zeDeviceGetCacheProperties");
    lib.load(api.device_pci_get_properties_ext, "zeDevicePciGetPropertiesExt");
    return api;
  }();
  return loaded ? &*loaded : nullptr;
}

// Enumerates handles with the usual Level Zero two-call pattern: fn(args..., &count, handles).
template <typename Handle, typename Fn, typename... Args>
std::vector<Handle> enumerate(Fn fn, Args... args) {
  std::uint32_t count = 0;
  if (fn == nullptr || fn(args..., &count, static_cast<Handle*>(nullptr)) != ZE_RESULT_SUCCESS || count == 0) {
    return {};
  }
  std::vector<Handle> handles(count);
  if (fn(args..., &count, handles.data()) != ZE_RESULT_SUCCESS) {
    return {};
  }
  handles.resize(count);
  return handles;
}

std::string_view memory_type(std::int32_t type) {
  constexpr std::array<std::string_view, 20> names{
      "HBM", "DDR", "DDR3", "DDR4", "DDR5",  "LPDDR", "LPDDR3", "LPDDR4", "LPDDR5", "SRAM",
      "L1",  "L3",  "GRF",  "SLM",  "GDDR4", "GDDR5", "GDDR5X", "GDDR6",  "GDDR6X", "GDDR7",
  };
  return type >= 0 && static_cast<std::size_t>(type) < names.size() ? names[static_cast<std::size_t>(type)]
                                                                    : std::string_view{};
}

// Memory type of the device local memory, from Sysman (matched by PCI address).
std::optional<std::string> sysman_memory_type(const std::string& address) {
  const SysmanApi* api = sysman();
  if (api == nullptr || api->memory_get_properties == nullptr) {
    return std::nullopt;
  }
  for (const auto driver : enumerate<zes_driver_handle_t>(api->driver_get)) {
    for (const auto device : enumerate<zes_device_handle_t>(api->device_get, driver)) {
      auto pci = with_stype<zes_pci_properties_t>(ZES_STRUCTURE_TYPE_PCI_PROPERTIES);
      if (api->device_pci_get_properties(device, &pci) != ZE_RESULT_SUCCESS ||
          format_pci_address(pci.address.domain, pci.address.bus, pci.address.device, pci.address.function) !=
              address) {
        continue;
      }
      for (const auto module : enumerate<zes_mem_handle_t>(api->device_enum_memory_modules, device)) {
        auto properties = with_stype<zes_mem_properties_t>(ZES_STRUCTURE_TYPE_MEM_PROPERTIES);
        if (api->memory_get_properties(module, &properties) == ZE_RESULT_SUCCESS &&
            properties.location == ZES_MEM_LOC_DEVICE) {
          if (const auto name = memory_type(properties.type); !name.empty()) {
            return std::string(name);
          }
        }
      }
    }
  }
  return std::nullopt;
}

Gpu read_device(const Core& api, ze_device_handle_t device, const ze_device_properties_t& properties,
                const std::optional<GpuApi>& version) {
  Gpu gpu;
  gpu.name = properties.name;
  gpu.uuid.emplace();
  std::ranges::copy(properties.uuid.id, gpu.uuid->bytes.begin());
  const bool integrated = (properties.flags & ZE_DEVICE_PROPERTY_FLAG_INTEGRATED) != 0;
  gpu.type = integrated ? GpuType::integrated : GpuType::discrete;
  if (properties.vendorId <= 0xffff && properties.deviceId <= 0xffff) {
    gpu.pci = PciDevice{.vendor_id = static_cast<std::uint16_t>(properties.vendorId),
                        .device_id = static_cast<std::uint16_t>(properties.deviceId)};
    if (auto pci = with_stype<ze_pci_ext_properties_t>(ZE_STRUCTURE_TYPE_PCI_EXT_PROPERTIES);
        api.device_pci_get_properties_ext && api.device_pci_get_properties_ext(device, &pci) == ZE_RESULT_SUCCESS) {
      gpu.pci->address =
          format_pci_address(pci.address.domain, pci.address.bus, pci.address.device, pci.address.function);
      if (pci.maxSpeed.genVersion > 0 && pci.maxSpeed.width > 0) {
        gpu.pci->max_link = PcieLink{.generation = static_cast<std::uint32_t>(pci.maxSpeed.genVersion),
                                     .width = static_cast<std::uint32_t>(pci.maxSpeed.width)};
      }
    }
  }

  const std::uint32_t subslices = properties.numSlices * properties.numSubslicesPerSlice;
  if (subslices > 0) {
    gpu.compute_units = subslices;  // Xe-cores
    if (const std::uint32_t lanes = subslices * properties.numEUsPerSubslice * properties.physicalEUSimdWidth) {
      gpu.cores = lanes;
    }
  }
  if (properties.coreClockRate > 0) {
    gpu.max_frequency = std::uint64_t{properties.coreClockRate} * FrequencyUnit::MHz;
  }
  // extension structures are queried one at a time: a driver may reject a chain with one it does not know
  if (auto ip = with_stype<ze_device_ip_version_ext_t>(ZE_STRUCTURE_TYPE_DEVICE_IP_VERSION_EXT); [&] {
        auto chained = with_stype<ze_device_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
        chained.pNext = &ip;
        return api.device_get_properties(device, &chained) == ZE_RESULT_SUCCESS;
      }()) {
    apply_intel_ip_version(gpu, ip.ipVersion);
  }
#ifdef HWINFO_WINDOWS
  if (auto luid = with_stype<ze_device_luid_ext_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_LUID_EXT_PROPERTIES); [&] {
        auto chained = with_stype<ze_device_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
        chained.pNext = &luid;
        return api.device_get_properties(device, &chained) == ZE_RESULT_SUCCESS;
      }()) {
    std::uint64_t value = 0;
    for (std::size_t i = 0; i < 8; ++i) {
      value |= std::uint64_t{luid.luid[i]} << (8 * i);
    }
    gpu.luid = value;
  }
#endif

  // the memory of integrated GPUs is the system RAM: only report it for discrete GPUs
  if (!integrated && api.device_get_memory_properties) {
    std::uint32_t count = 0;
    if (api.device_get_memory_properties(device, &count, nullptr) == ZE_RESULT_SUCCESS && count > 0) {
      std::vector<ze_device_memory_properties_t> memories(
          count, with_stype<ze_device_memory_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES));
      if (api.device_get_memory_properties(device, &count, memories.data()) == ZE_RESULT_SUCCESS) {
        std::uint64_t total = 0;
        for (const auto& memory : memories | std::views::take(count)) {
          total += memory.totalSize;
          if (memory.maxBusWidth > 0) {
            gpu.memory_bus_width = memory.maxBusWidth;
          }
          if (memory.maxClockRate > 0) {
            gpu.max_memory_frequency = std::uint64_t{memory.maxClockRate} * FrequencyUnit::MHz;
          }
        }
        if (total > 0) {
          gpu.dedicated_memory = Bytes{total};
        }
      }
    }
    if (gpu.pci && gpu.pci->address) {
      gpu.memory_type = sysman_memory_type(*gpu.pci->address);
    }
  }
  // the last level cache of the GPU (L3 on Intel, which takes the role of other vendors' L2)
  if (api.device_get_cache_properties) {
    std::uint32_t count = 0;
    if (api.device_get_cache_properties(device, &count, nullptr) == ZE_RESULT_SUCCESS && count > 0) {
      std::vector<ze_device_cache_properties_t> caches(
          count, with_stype<ze_device_cache_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_CACHE_PROPERTIES));
      if (api.device_get_cache_properties(device, &count, caches.data()) == ZE_RESULT_SUCCESS) {
        std::size_t largest = 0;
        for (const auto& cache : caches | std::views::take(count)) {
          largest = std::max(largest, cache.cacheSize);
        }
        if (largest > 0) {
          gpu.l2_cache = Bytes{largest};
        }
      }
    }
  }
  if (version) {
    gpu.compute_apis.push_back(*version);
  }
  return gpu;
}

}  // namespace

BackendResult level_zero_devices() {
  const Core* api = core();
  if (api == nullptr) {
    return {};
  }
  BackendResult result;
  for (const auto driver : enumerate<ze_driver_handle_t>(api->driver_get)) {
    std::optional<GpuApi> version;
    if (std::uint32_t v = 0;
        api->driver_get_api_version && api->driver_get_api_version(driver, &v) == ZE_RESULT_SUCCESS) {
      version = GpuApi{.name = "Level Zero", .version = std::format("{}.{}", v >> 16, v & 0xffff)};
    }
    for (const auto device : enumerate<ze_device_handle_t>(api->device_get, driver)) {
      auto properties = with_stype<ze_device_properties_t>(ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES);
      if (api->device_get_properties(device, &properties) == ZE_RESULT_SUCCESS &&
          properties.type == ZE_DEVICE_TYPE_GPU) {
        result.push_back(read_device(*api, device, properties, version));
      }
    }
  }
  return result;
}

}  // namespace gpu

}  // namespace hwinfo::internal

namespace hwinfo::internal::gpu {

namespace {

// Live data from Level Zero Sysman. Engine activity and energy are counters: their rates need two samples.
class LevelZeroSource final : public StatusSource {
 public:
  LevelZeroSource(const SysmanApi& api, zes_device_handle_t device) : _api(api), _device(device) {
    for (const auto engine : enumerate<zes_engine_handle_t>(api.device_enum_engine_groups, device)) {
      auto properties = with_stype<zes_engine_properties_t>(ZES_STRUCTURE_TYPE_ENGINE_PROPERTIES);
      if (api.engine_get_properties == nullptr || api.engine_get_properties(engine, &properties) != ZE_RESULT_SUCCESS ||
          properties.onSubdevice) {
        continue;
      }
      if (properties.type == ZES_ENGINE_GROUP_RENDER_ALL || properties.type == ZES_ENGINE_GROUP_COMPUTE_ALL) {
        _engines.push_back({engine, {}});
      } else if (properties.type == ZES_ENGINE_GROUP_MEDIA_ALL) {
        _media.push_back({engine, {}});
      } else if (properties.type == ZES_ENGINE_GROUP_ALL) {
        _all.push_back({engine, {}});
      }
    }
    if (_engines.empty()) {
      _engines = std::move(_all);
    }
    if (api.device_get_card_power_domain == nullptr ||
        api.device_get_card_power_domain(device, &_power) != ZE_RESULT_SUCCESS) {
      const auto domains = enumerate<zes_pwr_handle_t>(api.device_enum_power_domains, device);
      _power = domains.empty() ? nullptr : domains.front();
    }
    for (const auto sensor : enumerate<zes_temp_handle_t>(api.device_enum_temperature_sensors, device)) {
      auto properties = with_stype<zes_temp_properties_t>(ZES_STRUCTURE_TYPE_TEMP_PROPERTIES);
      if (api.temperature_get_properties && api.temperature_get_properties(sensor, &properties) == ZE_RESULT_SUCCESS &&
          !properties.onSubdevice &&
          (properties.type == ZES_TEMP_SENSORS_GPU || properties.type == ZES_TEMP_SENSORS_GLOBAL)) {
        _temperatures.push_back(sensor);
      }
    }
    for (const auto module : enumerate<zes_mem_handle_t>(api.device_enum_memory_modules, device)) {
      auto properties = with_stype<zes_mem_properties_t>(ZES_STRUCTURE_TYPE_MEM_PROPERTIES);
      if (api.memory_get_properties && api.memory_get_properties(module, &properties) == ZE_RESULT_SUCCESS &&
          !properties.onSubdevice && properties.location == ZES_MEM_LOC_DEVICE) {
        _memories.push_back(module);
      }
    }
    for (const auto domain : enumerate<zes_freq_handle_t>(api.device_enum_frequency_domains, device)) {
      auto properties = with_stype<zes_freq_properties_t>(ZES_STRUCTURE_TYPE_FREQ_PROPERTIES);
      if (api.frequency_get_properties && api.frequency_get_properties(domain, &properties) == ZE_RESULT_SUCCESS &&
          !properties.onSubdevice) {
        if (properties.type == ZES_FREQ_DOMAIN_GPU && !_gpu_frequency) {
          _gpu_frequency = domain;
        } else if (properties.type == ZES_FREQ_DOMAIN_MEMORY && !_memory_frequency) {
          _memory_frequency = domain;
        }
      }
    }
  }

  void sample(GpuStatus& status) override {
    fill(status.utilization, busy(_engines));
    fill(status.video_utilization, busy(_media));
    if (zes_power_energy_counter_t energy{}; _power && _api.power_get_energy_counter &&
                                             _api.power_get_energy_counter(_power, &energy) == ZE_RESULT_SUCCESS) {
      if (const auto watts = _energy.update(energy.energy, energy.timestamp)) {  // uJ / us = W
        fill(status.power, std::optional(Power{static_cast<std::uint64_t>(*watts * 1e6)}));
      }
    }
    std::optional<double> temperature;
    for (const auto sensor : _temperatures) {
      if (double celsius = 0; _api.temperature_get_state(sensor, &celsius) == ZE_RESULT_SUCCESS && celsius > 0) {
        temperature = std::max(temperature.value_or(celsius), celsius);
      }
    }
    fill(status.temperature, temperature);
    std::uint64_t total = 0;
    std::uint64_t free = 0;
    for (const auto module : _memories) {
      if (auto state = with_stype<zes_mem_state_t>(ZES_STRUCTURE_TYPE_MEM_STATE);
          _api.memory_get_state && _api.memory_get_state(module, &state) == ZE_RESULT_SUCCESS) {
        total += state.size;
        free += state.free;
      }
    }
    if (total > 0) {
      fill(status.memory_total, std::optional(Bytes{total}));
      fill(status.memory_used, std::optional(Bytes{total - std::min(free, total)}));
    }
    fill(status.frequency, frequency(_gpu_frequency));
    fill(status.memory_frequency, frequency(_memory_frequency));
    if (auto pci = with_stype<zes_pci_state_t>(ZES_STRUCTURE_TYPE_PCI_STATE);
        _api.device_pci_get_state && _api.device_pci_get_state(_device, &pci) == ZE_RESULT_SUCCESS &&
        pci.speed.gen > 0 && pci.speed.width > 0) {
      fill(status.pcie_link, std::optional(PcieLink{.generation = static_cast<std::uint32_t>(pci.speed.gen),
                                                    .width = static_cast<std::uint32_t>(pci.speed.width)}));
    }
  }

 private:
  struct Engine {
    zes_engine_handle_t handle;
    CounterRate activity;
  };

  // Highest activity of the engine groups since the previous sample.
  std::optional<double> busy(std::vector<Engine>& engines) {
    std::optional<double> result;
    for (auto& engine : engines) {
      zes_engine_stats_t stats{};
      if (_api.engine_get_activity == nullptr || _api.engine_get_activity(engine.handle, &stats) != ZE_RESULT_SUCCESS) {
        continue;
      }
      if (const auto rate = engine.activity.update(stats.activeTime, stats.timestamp)) {
        result = std::max(result.value_or(0), std::min(*rate, 1.0));
      }
    }
    return result;
  }

  std::optional<Hertz> frequency(zes_freq_handle_t domain) const {
    auto state = with_stype<zes_freq_state_t>(ZES_STRUCTURE_TYPE_FREQ_STATE);
    if (domain == nullptr || _api.frequency_get_state == nullptr ||
        _api.frequency_get_state(domain, &state) != ZE_RESULT_SUCCESS || state.actual <= 0) {
      return std::nullopt;
    }
    return static_cast<std::uint64_t>(state.actual) * FrequencyUnit::MHz;
  }

  const SysmanApi& _api;
  zes_device_handle_t _device;
  std::vector<Engine> _engines;  // render / compute (or all engines combined)
  std::vector<Engine> _media;
  std::vector<Engine> _all;
  zes_pwr_handle_t _power = nullptr;
  CounterRate _energy;
  std::vector<zes_temp_handle_t> _temperatures;
  std::vector<zes_mem_handle_t> _memories;
  zes_freq_handle_t _gpu_frequency = nullptr;
  zes_freq_handle_t _memory_frequency = nullptr;
};

}  // namespace

std::unique_ptr<StatusSource> level_zero_source(const Gpu& gpu) {
  if (!gpu.pci || !gpu.pci->address || gpu.pci->vendor_id != pci_vendor::intel) {
    return nullptr;  // only Intel ships a Level Zero driver
  }
  const SysmanApi* api = sysman();
  if (api == nullptr) {
    return nullptr;
  }
  for (const auto driver : enumerate<zes_driver_handle_t>(api->driver_get)) {
    for (const auto device : enumerate<zes_device_handle_t>(api->device_get, driver)) {
      auto pci = with_stype<zes_pci_properties_t>(ZES_STRUCTURE_TYPE_PCI_PROPERTIES);
      if (api->device_pci_get_properties(device, &pci) == ZE_RESULT_SUCCESS &&
          format_pci_address(pci.address.domain, pci.address.bus, pci.address.device, pci.address.function) ==
              *gpu.pci->address) {
        return std::make_unique<LevelZeroSource>(*api, device);
      }
    }
  }
  return nullptr;
}

}  // namespace hwinfo::internal::gpu
