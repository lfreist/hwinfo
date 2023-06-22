// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <string>
#include <vector>
#include <algorithm>

#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<std::string> getVendor() {
  std::vector<const wchar_t*> vendor{};
  wmi::queryWMI("WIN32_VideoController", "AdapterCompatibility", vendor);
  std::vector<std::string> ret{};
  ret.reserve(vendor.size());
  for (auto& v : vendor) {
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
      ret.push_back(static_cast<int64_t>(memory[0]) * 2);
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
    auto vendors = getVendor();
    auto names = getName();
    auto driver = getDriverVersion();
    auto memory = getMemory_Bytes();
    size_t min = 0xffff;
    min = std::min(min, vendors.size());
    std::vector<GPU> gpus;
    for (int i = 0; i < vendors.size()) {
        GPU gpu;
        gpu._name = names[i];
        gpu._vendor = vendors[i];
    }
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS