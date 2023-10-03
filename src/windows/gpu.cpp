// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")

#include <hwinfo/gpu.h>
#include <hwinfo/utils/utils.h>
#include <hwinfo/utils/stringutils.h>

namespace hwinfo {
namespace gpu {

// _____________________________________________________________________________________________________________________
std::vector<std::string> getVendor() {
  std::vector<const wchar_t*> vendor{};
  wmi::queryWMI("WIN32_VideoController", "AdapterCompatibility", vendor);
  std::vector<std::string> ret{};
  ret.reserve(vendor.size());
  for (auto& v : vendor) {
    if (v == nullptr) {
      continue;
    }
    std::wstring tmp(v);
    ret.emplace_back(utils::wstring_to_std_string(tmp));
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<std::string> getName() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_VideoController", "Name", names);
  std::vector<std::string> ret{};
  ret.reserve(names.size());
  for (auto& v : names) {
    if (v == nullptr) {
      continue;
    }
    std::wstring tmp(v);
    ret.emplace_back(utils::wstring_to_std_string(tmp));
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<std::string> getDriverVersion() {
  std::vector<const wchar_t*> driverVersion{};
  wmi::queryWMI("WIN32_VideoController", "DriverVersion", driverVersion);
  std::vector<std::string> ret{};
  ret.reserve(driverVersion.size());
  for (auto& v : driverVersion) {
    if (v == nullptr) {
      continue;
    }
    std::wstring tmp(v);
    ret.emplace_back(utils::wstring_to_std_string(tmp));
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> getMemory_Bytes() {
  std::vector<unsigned long long> memory{};
  wmi::queryWMI("WIN32_VideoController", "AdapterRam", memory);
  std::vector<int64_t> ret;
  ret.reserve(memory.size());
  for (auto v : memory) {
    ret.push_back(static_cast<int64_t>(v) * 2);
  }
  return ret;
}

}  // namespace gpu

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  auto vendors = gpu::getVendor();
  auto names = gpu::getName();
  auto driver = gpu::getDriverVersion();
  auto memory = gpu::getMemory_Bytes();
  std::vector<GPU> gpus;
  for (size_t i = 0; i < vendors.size(); ++i) {
    GPU gpu;
    gpu._name = ::utils::get_value(names, i);
    gpu._vendor = ::utils::get_value(vendors, i);
    gpu._driverVersion = ::utils::get_value(driver, i);
    gpu._memory_Bytes = ::utils::get_value(memory, i);
    gpus.push_back(std::move(gpu));
  }
#ifdef USE_OCL
  auto cl_gpus = mcl::DeviceManager::get_list<mcl::Filter::GPU>();
  for (auto& gpu : gpus) {
    for (auto* cl_gpu : cl_gpus) {
      if (cl_gpu->name().find(gpu._device_id)) {
        gpu._driverVersion = cl_gpu->driver_version();
        gpu._frequency_MHz = static_cast<int64_t>(cl_gpu->clock_frequency_MHz());
        gpu._num_cores = static_cast<int>(cl_gpu->cores());
        gpu._memory_Bytes = static_cast<int64_t>(cl_gpu->memory_Bytes());
      }
    }
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
