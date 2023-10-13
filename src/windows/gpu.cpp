// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")

#include <hwinfo/gpu.h>
#include <hwinfo/utils/stringutils.h>
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
  utils::WMI::_WMI wmi;
  const std::wstring query_string(
      L"SELECT Name, AdapterCompatibility, DriverVersion, AdapterRam "
      L"FROM WIN32_VideoController");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<GPU> gpus;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int gpu_id = 0;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    GPU gpu;
    gpu._id = gpu_id++;
    VARIANT vt_prop;
    obj->Get(L"Name", 0, &vt_prop, NULL, NULL);
    gpu._name = utils::wstring_to_std_string(vt_prop.bstrVal);
    obj->Get(L"AdapterCompatibility", 0, &vt_prop, NULL, NULL);
    gpu._vendor = utils::wstring_to_std_string(vt_prop.bstrVal);
    obj->Get(L"DriverVersion", 0, &vt_prop, NULL, NULL);
    gpu._driverVersion = utils::wstring_to_std_string(vt_prop.bstrVal);
    obj->Get(L"AdapterRam", 0, &vt_prop, NULL, NULL);
    gpu._memory_Bytes = vt_prop.intVal;
    VariantClear(&vt_prop);
    obj->Release();
    gpus.push_back(std::move(gpu));
  }
#ifdef USE_OCL
  auto cl_gpus = mcl::DeviceManager::get_list<mcl::Filter::GPU>();
  for (auto& gpu : gpus) {
    for (auto* cl_gpu : cl_gpus) {
      if (cl_gpu->name() == gpu.name()) {
        gpu._driverVersion = cl_gpu->driver_version();
        gpu._frequency_MHz = static_cast<int64_t>(cl_gpu->clock_frequency_MHz());
        gpu._num_cores = static_cast<int>(cl_gpu->cores());
        gpu._memory_Bytes = static_cast<int64_t>(cl_gpu->memory_Bytes());
        break;
      }
    }
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
