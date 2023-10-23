// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/cpu.h>
#include <hwinfo/cpuid.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <algorithm>
#include <string>
#include <vector>

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
int64_t CPU::currentClockSpeed_MHz(int thread_id) const {
  auto data = utils::WMI::query<std::string>(L"Win32_PerfFormattedData_Counters_ProcessorInformation",
                                             L"PercentProcessorPerformance");
  if (data.empty()) {
    return -1;
  }
  double performance = std::stod(data[thread_id]) / 100;
  return static_cast<int64_t>(static_cast<double>(_maxClockSpeed_MHz) * performance);
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> CPU::currentClockSpeed_MHz() const {
  std::vector<int64_t> result;
  result.reserve(_numLogicalCores);
  auto data = utils::WMI::query<std::string>(L"Win32_PerfFormattedData_Counters_ProcessorInformation",
                                             L"PercentProcessorPerformance");
  if (data.empty()) {
    result.resize(_numLogicalCores, -1);
    return result;
  }
  for (auto& v : data) {
    double performance = std::stod(v) / 100;
    result.push_back(static_cast<int64_t>(static_cast<double>(_maxClockSpeed_MHz) * performance));
  }
  return result;
}

double CPU::currentUtilisation() const {
  auto res = utils::WMI::query<std::string>(L"Win32_PerfFormattedData_Counters_ProcessorInformation",
                                            L"PercentProcessorUtility", L"Name=" + std::to_wstring(0) + L",_Total");
  if (res.empty()) {
    return -1.f;
  }
  return std::stod(res[0]);
}

double CPU::threadUtilisation(int thread_id) const {
  auto data = utils::WMI::query<std::string>(L"Win32_PerfFormattedData_Counters_ProcessorInformation",
                                             L"PercentProcessorUtility");
  if (data.empty()) {
    return -1.f;
  }
  std::string thread_value = data[thread_id];
  if (thread_value.empty()) {
    return -1.f;
  }
  return std::stod(thread_value);
}

std::vector<double> CPU::threadsUtilisation() const {
  std::vector<double> thread_utility;
  thread_utility.reserve(_numLogicalCores);
  auto data = utils::WMI::query<std::string>(L"Win32_PerfFormattedData_Counters_ProcessorInformation",
                                             L"PercentProcessorUtility");
  if (data.empty()) {
    thread_utility.resize(_numLogicalCores, -1.f);
    return thread_utility;
  }
  for (const auto& v : data) {
    if (v.empty()) {
      thread_utility.push_back(-1.f);
    } else {
      thread_utility.push_back(std::stod(v) / 100.f);
    }
  }
  return thread_utility;
}

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<CPU> getAllCPUs() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(
      L"SELECT Name, Manufacturer, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed, L2CacheSize, L3CacheSize "
      L"FROM Win32_Processor");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<CPU> cpus;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int cpu_id = 0;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    CPU cpu;
    cpu._id = cpu_id++;
    VARIANT vt_prop;
    HRESULT hr;
    hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      cpu._modelName = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"Manufacturer", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      cpu._vendor = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"NumberOfCores", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      cpu._numPhysicalCores = vt_prop.intVal;
    }
    hr = obj->Get(L"NumberOfLogicalProcessors", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      cpu._numLogicalCores = vt_prop.intVal;
    }
    hr = obj->Get(L"MaxClockSpeed", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      cpu._maxClockSpeed_MHz = vt_prop.uintVal;
      cpu._regularClockSpeed_MHz = vt_prop.uintVal;
    }
    VariantClear(&vt_prop);
    obj->Release();
    auto cache{[&]() -> void {
      auto data = utils::WMI::query<std::uint32_t>(L"Win32_CacheMemory", L"MaxCacheSize");
      if (data.empty()) {
        cpu._L1CacheSize_Bytes = cpu._L2CacheSize_Bytes = cpu._L3CacheSize_Bytes = -1;
      }
      try {
        cpu._L1CacheSize_Bytes = data.at(0);
        cpu._L2CacheSize_Bytes = data.at(1);
        cpu._L3CacheSize_Bytes = data.at(2);
      } catch (const std::exception& e) {
        std::cout << "Exception in CPU Cache: " << e.what() << std::endl;
      }
    }};
    cache();
    cpus.push_back(std::move(cpu));
  }
  return cpus;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
