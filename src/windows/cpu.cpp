// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>
#include <hwinfo/cpu.h>
#include <hwinfo/cpuid.h>
#include <hwinfo/utils/utils.h>
#include <hwinfo/utils/stringutils.h>

#include <algorithm>
#include <string>
#include <thread>
#include <vector>

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
// TODO: rewrite
int64_t CPU::currentClockSpeed_MHz() const {
  // Intel Turbo Boost Support -> https://stackoverflow.com/a/61808781
  // It's actually a string which holds the percentage ->
  // https://wutils.com/wmi/root/cimv2/win32_perfformatteddata_counters_processorinformation/instances.html
  std::vector<bstr_t> performance{};
  wmi::queryWMI("Win32_PerfFormattedData_Counters_ProcessorInformation", "PercentProcessorPerformance", performance);
  if (!performance.empty()) {
    const char* strValue = static_cast<const char*>(performance[0]);
    double performance_perc = std::stod(strValue);

    if (performance_perc > 0) {
      std::vector<int64_t> maxSpeed{};
      wmi::queryWMI("Win32_Processor", "MaxClockSpeed", maxSpeed);
      if (maxSpeed.empty()) {
        return -1;
      }

      // This basic calcuation does the trick...
      return maxSpeed[0] * performance_perc / 100;
    }
  }

  // As Fallback we take the Win32_Processor.CurrentClockSpeed
  std::vector<int64_t> speed{};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speed);
  if (speed.empty()) {
    return -1;
  }
  return speed[0];
}

double CPU::currentUtility_Percentage() const {
  std::vector<bstr_t> percentage{};
  const std::string& query =
      "Win32_PerfFormattedData_Counters_ProcessorInformation WHERE Name='" + std::to_string(0) + ",_Total'";
  wmi::queryWMI(query, "PercentProcessorUtility", percentage);
  if (percentage.empty()) {
    return -1.0;
  }

  const char* strValue = static_cast<const char*>(percentage[0]);
  return std::stod(strValue);
}

double CPU::currentThreadUtility_Percentage(int thread_index) const {
  std::vector<bstr_t> percentage{};
  const std::string& query = "Win32_PerfFormattedData_Counters_ProcessorInformation WHERE Name='" + std::to_string(0) +
                       "," + std::to_string(thread_index) + "'";
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
    threads.emplace_back(
        [&, thread_idx]() { thread_utility[thread_idx] = currentThreadUtility_Percentage(thread_idx); });
  }

  // Join the threads
  for (auto& thread : threads) {
    thread.join();
  }
  return thread_utility;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS