// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>
#include <hwinfo/cpu.h>
#include <hwinfo/cpuid.h>
#include <hwinfo/utils/utils.h>

#include <algorithm>
#include <string>
#include <vector>
#include <thread>

namespace hwinfo {
namespace cpu {

// _____________________________________________________________________________________________________________________
std::vector<std::string> getVendor() {
  std::vector<const wchar_t*> vendors{};
  wmi::queryWMI("Win32_Processor", "Manufacturer", vendors);
  std::vector<std::string> ret;
  ret.reserve(vendors.size());
  for (auto& v : vendors) {
    if (v == nullptr) {
      continue;
    }
    std::wstring tmp(v);
    ret.emplace_back(tmp.begin(), tmp.end());
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<std::string> getModelName() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("Win32_Processor", "Name", names);
  std::vector<std::string> ret;
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
std::vector<int64_t> getNumLogicalCores() {
  std::vector<int64_t> cores{};
  wmi::queryWMI("Win32_Processor", "NumberOfLogicalProcessors", cores);
  std::vector<int64_t> ret;
  ret.reserve(cores.size());
  for (auto& v : cores) {
    ret.push_back(v);
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> getNumPhysicalCores() {
  std::vector<int64_t> cores{};
  wmi::queryWMI("Win32_Processor", "NumberOfCores", cores);
  std::vector<int64_t> ret;
  ret.reserve(cores.size());
  for (auto& v : cores) {
    ret.push_back(v);
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> getMaxClockSpeed_MHz() {
  std::vector<int64_t> speeds{};
  wmi::queryWMI("Win32_Processor", "MaxClockSpeed", speeds);
  std::vector<int64_t> ret;
  ret.reserve(speeds.size());
  for (auto& v : speeds) {
    ret.push_back(v);
  }
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> getRegularClockSpeed_MHz() {
  std::vector<int64_t> speeds{};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speeds);
  std::vector<int64_t> ret;
  ret.reserve(speeds.size());
  for (auto& v : speeds) {
    ret.push_back(v);
  }
  return ret;
}

std::vector<int64_t> getCacheSize_Bytes() {
  std::vector<int64_t> cacheSizes{};
  wmi::queryWMI("Win32_Processor", "L3CacheSize", cacheSizes);
  std::vector<int64_t> ret;
  ret.reserve(cacheSizes.size());
  for (auto& v : cacheSizes) {
    ret.push_back(v * 1024);
  }
  return ret;
}
}  // namespace cpu

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
int64_t CPU::currentClockSpeed_MHz() const {
  // Intel Turbo Boost Support -> https://stackoverflow.com/a/61808781
  // It's actually a string which holds the percentage -> https://wutils.com/wmi/root/cimv2/win32_perfformatteddata_counters_processorinformation/instances.html
  std::vector<bstr_t> performance{};
  wmi::queryWMI("Win32_PerfFormattedData_Counters_ProcessorInformation", "PercentProcessorPerformance", performance);
  if (!performance.empty())
  {
    const char* strValue = static_cast<const char*>(performance[_core_id]);
    double performance_perc = std::stod(strValue);

    if (performance_perc > 0)
    {
        std::vector<int64_t> maxSpeed{};
        wmi::queryWMI("Win32_Processor", "MaxClockSpeed", maxSpeed);
        if (maxSpeed.empty()) {
          return -1;
        }

      // This basic calcuation does the trick...
      return maxSpeed[_core_id] * performance_perc / 100;
    }
  }

  // As Fallback we take the Win32_Processor.CurrentClockSpeed
  std::vector<int64_t> speed{};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speed);
  if (speed.empty()) {
    return -1;
  }
  return speed[_core_id];
}

double CPU::currentUtility_Percentage() const {
  std::vector<bstr_t> percentage{};
  std::string& query = "Win32_PerfFormattedData_Counters_ProcessorInformation WHERE Name='" + std::to_string(_core_id) + ",_Total'";
  wmi::queryWMI(query, "PercentProcessorUtility", percentage);
  if (percentage.empty()) {
    return -1.0;
  }

  const char* strValue = static_cast<const char*>(percentage[0]);
  return std::stod(strValue);
}

double CPU::currentThreadUtility_Percentage(const int& thread_index) const {
  std::vector<bstr_t> percentage{};
  std::string& query = "Win32_PerfFormattedData_Counters_ProcessorInformation WHERE Name='" + std::to_string(_core_id) + "," + std::to_string(thread_index) + "'";
  wmi::queryWMI(query, "PercentProcessorUtility", percentage);
  if (percentage.empty()) {
    return -1.0;
  }

  if (SysStringLen(percentage[0]) == 0) {
    return -1.0;
  }

  const char* strValue = static_cast<const char*>(percentage[0]);
  if (strValue) {
    return std::stod(strValue);
  }
  return -1.0;
}

 std::vector<double> CPU::currentThreadsUtility_Percentage_MainThread() const {
  std::vector<double> thread_utility(CPU::_numLogicalCores);
  std::vector<std::thread> threads;

  for (int thread_idx = 0; thread_idx < CPU::_numLogicalCores; ++thread_idx) {
    threads.emplace_back([&, thread_idx]() {
      thread_utility[thread_idx] = currentThreadUtility_Percentage(thread_idx);
    });
  }

  // Join the threads
  for (auto& thread : threads) {
    thread.join();
  }
  return thread_utility;
 }

// Might requires https://github.com/LibreHardwareMonitor/LibreHardwareMonitor | It's a good library, however you need to call the C# function in C++, but that's defenity something to conside, might be useful for GPU as well
//  double CPU::currentTemperature_Celsius() const {
//   return -1.0;
//  }

// _____________________________________________________________________________________________________________________
std::vector<Socket> getAllSockets() {
  std::vector<Socket> sockets;
  auto vendors = cpu::getVendor();
  auto names = cpu::getModelName();
  auto cache_sizes = cpu::getCacheSize_Bytes();
  auto phys_cores = cpu::getNumPhysicalCores();
  auto logical_cores = cpu::getNumLogicalCores();
  auto max_speed = cpu::getMaxClockSpeed_MHz();
  auto regular_speed = cpu::getRegularClockSpeed_MHz();

  for (size_t i = 0; i < vendors.size(); ++i) {
    CPU cpu;
    cpu._core_id = static_cast<int>(i);
    cpu._cacheSize_Bytes = utils::get_value(cache_sizes, i);
    cpu._maxClockSpeed_MHz = utils::get_value(max_speed, i);
    cpu._minClockSpeed_MHz = -1;
    cpu._regularClockSpeed_MHz = utils::get_value(regular_speed, i);
    cpu._modelName = utils::get_value(names, i);
    cpu._vendor = utils::get_value(vendors, i);
    cpu._numLogicalCores = static_cast<int>(utils::get_value(logical_cores, i));
    cpu._numPhysicalCores = static_cast<int>(utils::get_value(phys_cores, i));
    Socket socket(cpu);
    socket._id = static_cast<int>(i);
    sockets.push_back(std::move(socket));
  }

  return sockets;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS