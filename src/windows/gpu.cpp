// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>

#include <algorithm>
#include <string>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")

#include <hwinfo/gpu.h>
#include <hwinfo/utils/utils.h>

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
    ret.emplace_back(tmp.begin(), tmp.end());
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
    ret.emplace_back(tmp.begin(), tmp.end());
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
    ret.emplace_back(tmp.begin(), tmp.end());
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
    gpu._name = utils::get_value(names, i);
    gpu._vendor = utils::get_value(vendors, i);
    gpu._driverVersion = utils::get_value(driver, i);
    gpu._memory_Bytes = utils::get_value(memory, i);
    gpus.push_back(std::move(gpu));
  }
#ifdef USE_OCL
  auto cl_gpus = get_cpu_cl_data();
  // Windows
  auto min = min(cl_gpus.size(), gpus.size());
  for (int i = 0; i < min; ++i) {
    gpus[i]._num_cores = cl_gpus[i].num_cores;
    gpus[i]._frequency_MHz = cl_gpus[i].frequency_MHz;
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
