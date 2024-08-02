// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/opencl/device.h>

#include <algorithm>

namespace opencl_ {

Device::Device(uint32_t id, cl::Device cl_device) : _cl_device(std::move(cl_device)), _id(id) {
  _cores = _compute_cores();
  _instructions_per_cycle = type() == Type::GPU ? 2 : 32;
}
Device::~Device() = default;

Device::Device(Device&& device) noexcept
    : _instructions_per_cycle(device._instructions_per_cycle),
      _cores(device._cores),
      _id(device._id),
      _intel_gt_4gb_buffer_required(device._intel_gt_4gb_buffer_required),
      _memory_used_Bytes(device._memory_used_Bytes),
      _cl_device(std::move(device._cl_device)) {}

Device& Device::operator=(opencl_::Device&& device) noexcept {
  _cl_device = std::move(device._cl_device);
  _intel_gt_4gb_buffer_required = device._intel_gt_4gb_buffer_required;
  _id = device._id;
  _cores = device._cores;
  _instructions_per_cycle = device._instructions_per_cycle;
  _memory_used_Bytes = device._memory_used_Bytes;
  return *this;
}

uint32_t Device::get_id() const { return _id; }

const cl::Device& Device::get_cl_device() const { return _cl_device; }

cl::Device& Device::get_cl_device() { return _cl_device; }

std::string Device::name() const { return _cl_device.getInfo<CL_DEVICE_NAME>(); }

std::string Device::vendor() const { return _cl_device.getInfo<CL_DEVICE_VENDOR>(); }

std::string Device::driver_version() const { return _cl_device.getInfo<CL_DRIVER_VERSION>(); }

std::string Device::opencl_c_version() const { return _cl_device.getInfo<CL_DEVICE_OPENCL_C_VERSION>(); }

uint64_t Device::memory_Bytes() const { return _cl_device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>(); }

uint64_t Device::memory_used_Bytes() const { return _memory_used_Bytes; }

uint64_t Device::global_cache_Bytes() const { return _cl_device.getInfo<CL_DEVICE_GLOBAL_MEM_CACHE_SIZE>(); }

uint64_t Device::local_cache_Bytes() const { return _cl_device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>(); }

uint64_t Device::max_global_buffer_Bytes() const { return _cl_device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>(); }

uint64_t Device::max_constant_buffer_Bytes() const { return _cl_device.getInfo<CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE>(); }

uint64_t Device::compute_units() const { return _cl_device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(); }

uint64_t Device::cores() const { return _cores; }

uint64_t Device::clock_frequency_MHz() const { return _cl_device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>(); }

Device::Type Device::type() const {
  if (_cl_device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU) {
    return Type::CPU;
  }
  return Type::GPU;
}

uint64_t Device::fp64() const {
  return _cl_device.getInfo<CL_DEVICE_EXTENSIONS>().find("cl_khr_fp64") != std::string::npos
             ? _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE>()
             : 0;
}

uint64_t Device::fp32() const { return _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT>(); }

uint64_t Device::fp16() const {
  return _cl_device.getInfo<CL_DEVICE_EXTENSIONS>().find("cl_khr_fp16") != std::string::npos
             ? _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF>()
             : 0;
}

uint64_t Device::int64() const { return _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG>(); }

uint64_t Device::int32() const { return _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_INT>(); }

uint64_t Device::int16() const { return _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT>(); }

uint64_t Device::int8() const { return _cl_device.getInfo<CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR>(); }

uint64_t Device::estimated_flops() const {
  return (cores() * _instructions_per_cycle * clock_frequency_MHz() * 1000 * 1000);
}

bool Device::intel_gt_4gb_buffer_required() const { return _intel_gt_4gb_buffer_required; }

uint64_t Device::_compute_cores() {
  auto device_name = name();
  auto device_vendor = vendor();
  std::transform(device_name.begin(), device_name.end(), device_name.begin(), [](char c) { return std::tolower(c); });
  std::transform(device_vendor.begin(), device_vendor.end(), device_vendor.begin(),
                 [](char c) { return std::tolower(c); });
  if (device_vendor.find("nvidia") != std::string::npos) {
    // NVIDIA GPU
    if (std::any_of(nvidia_192.begin(), nvidia_192.end(),
                    [&device_name](const std::string& val) { return device_name.find(val) != std::string::npos; })) {
      return compute_units() * 192;
    }
    if (clock_frequency_MHz() < 1000 && device_name.find("titan") != std::string::npos) {
      return compute_units() * 192;
    }
    if (std::any_of(nvidia_64.begin(), nvidia_64.end(),
                    [&device_name](const std::string& val) { return device_name.find(val) != std::string::npos; })) {
      if (device_name.find("rtx a") != std::string::npos) {
        return compute_units() * 128;
      }
      return compute_units() * 192;
    }
    return compute_units() * 128;
  } else if (device_vendor.find("amd") != std::string::npos) {
    // AMD GPU
    if (type() == Type::CPU) {
      return compute_units() / 2;
    }
    if (device_name.find("gfx10") != std::string::npos) {
      return compute_units() * 128;
    }
    if (device_name.find("gfx11") != std::string::npos) {
      return compute_units() * 256;
    }
    return compute_units() * 64;
  } else if (device_vendor.find("intel") != std::string::npos) {
    // intel GPU
    if (type() == Type::CPU) {
      return compute_units() / 2;
    }
    if (device_name.find("gpu max") != std::string::npos) {
      return compute_units() * 16;
    }
    if (memory_Bytes() >= 0x100000000) {
      _intel_gt_4gb_buffer_required = true;
    }
    return compute_units() * 8;
  } else if (device_vendor.find("apple") != std::string::npos) {
    // Apple GPU
    return compute_units() * 128;
  } else if (device_vendor.find("arm") != std::string::npos) {
    // ARM GPU
    if (type() == Device::CPU) {
      return compute_units();
    }
    return compute_units() * 8;
  }
  return compute_units();
}

std::ostream& operator<<(std::ostream& os, const Device& device) {
  os << device.type() << device.name() << " (" << device.compute_units() << " CU [" << device.cores() << " Cores])";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Device::Type& type) {
  os << (type == Device::Type::GPU ? "GPU: " : "CPU: ");
  return os;
}

// ===== DeviceManager =================================================================================================

template <>
Device* DeviceManager::get<Filter::ID>(uint32_t value) {
  auto& dm = DeviceManager::get_instance();
  if (value >= dm._devices.size()) {
    throw std::runtime_error("Device with id " + std::to_string(value) + " not available.");
  }
  return &dm._devices[value];
}

template <>
Device* DeviceManager::get<Filter::MAX_MEMORY>() {
  auto& dm = DeviceManager::get_instance();
  return &(*std::max_element(dm._devices.begin(), dm._devices.end(),
                             [](const Device& a, const Device& b) { return a.memory_Bytes() < b.memory_Bytes(); }));
}

template <>
Device* DeviceManager::get<Filter::MIN_MEMORY>() {
  auto& dm = DeviceManager::get_instance();

  return &(*std::min_element(dm._devices.begin(), dm._devices.end(),
                             [](const Device& a, const Device& b) { return a.memory_Bytes() < b.memory_Bytes(); }));
}

template <>
Device* DeviceManager::get<Filter::MAX_FLOPS>() {
  auto& dm = DeviceManager::get_instance();

  return &(*std::max_element(dm._devices.begin(), dm._devices.end(), [](const Device& a, const Device& b) {
    return a.estimated_flops() < b.estimated_flops();
  }));
}

template <>
Device* DeviceManager::get<Filter::MIN_FLOPS>() {
  auto& dm = DeviceManager::get_instance();
  return &(*std::min_element(dm._devices.begin(), dm._devices.end(), [](const Device& a, const Device& b) {
    return a.estimated_flops() < b.estimated_flops();
  }));
}

template <>
std::vector<Device*> DeviceManager::get_list<Filter::ALL>() {
  auto& dm = DeviceManager::get_instance();
  std::vector<Device*> all;
  all.reserve(dm._devices.size());
  for (auto& device : dm._devices) {
    all.push_back(&device);
  }
  return all;
}

template <>
std::vector<Device*> DeviceManager::get_list<Filter::GPU>() {
  auto& dm = DeviceManager::get_instance();
  std::vector<Device*> gpu_devices;
  for (auto& device : dm._devices) {
    if (device.type() == Device::Type::GPU) {
      gpu_devices.push_back(&device);
    }
  }
  return gpu_devices;
}

template <>
std::vector<Device*> DeviceManager::get_list<Filter::CPU>() {
  auto& dm = DeviceManager::get_instance();
  std::vector<Device*> cpu_devices;
  for (auto& device : dm._devices) {
    if (device.type() == Device::Type::CPU) {
      cpu_devices.push_back(&device);
    }
  }
  return cpu_devices;
}

DeviceManager& DeviceManager::get_instance() {
  static DeviceManager device_manager;
  return device_manager;
}

DeviceManager::DeviceManager() {
  std::vector<cl::Platform> cl_platforms;
  cl::Platform::get(&cl_platforms);
  uint32_t id = 0;
  for (const auto& clp : cl_platforms) {
    std::vector<cl::Device> cl_devices;
    clp.getDevices(CL_DEVICE_TYPE_ALL, &cl_devices);
    for (auto& cld : cl_devices) {
      _devices.emplace_back(id++, std::move(cld));
    }
  }
}

}  // namespace opencl_