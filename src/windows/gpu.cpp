// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/gpu.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <algorithm>
#include <string>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")

#ifdef USE_OCL
#include <missocl/opencl.h>
#endif

namespace hwinfo {

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
    HRESULT hr;
    hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      gpu._name = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"AdapterCompatibility", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      gpu._vendor = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"DriverVersion", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      gpu._driverVersion = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"AdapterRam", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      gpu._memory_Bytes = vt_prop.uintVal;
    }
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
