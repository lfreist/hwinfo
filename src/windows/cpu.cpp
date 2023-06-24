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
  std::vector<int64_t> speed{};
  wmi::queryWMI("Win32_Processor", "CurrentClockSpeed", speed);
  if (speed.empty()) {
    return -1;
  }
  return speed[_core_id];
}

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