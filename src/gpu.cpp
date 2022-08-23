// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <Windows.h>
#include <comdef.h>
#include <WbemIdl.h>
#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")

#endif

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
GPU::GPU() {
  _name = getName();
  _driverVersion = getDriverVersion();
  _memoryMiB = getMemoryMiB();
}

// _____________________________________________________________________________________________________________________
GPU::~GPU() = default;

// _____________________________________________________________________________________________________________________
std::string GPU::getName() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return "<unknown>";
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_VideoController", "Name", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return std::string(tmp.begin(), tmp.end());
#else
#error "unsupported platform"
#endif
}

// _____________________________________________________________________________________________________________________
std::string GPU::getDriverVersion() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return "<unknown>";
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> driverVersion{};
  wmi::queryWMI("WIN32_VideoController", "DriverVersion", driverVersion);
  auto ret = driverVersion[0];
  std::wstring tmp(ret);
  return std::string(tmp.begin(), tmp.end());
#else
#error "unsupported platform"
#endif
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemoryMiB() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return -1;
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<unsigned long long> memory{};
  wmi::queryWMI("WIN32_VideoController", "AdapterRam", memory);
  return static_cast<int>(memory[0] * 2 / (1024 * 1024));
#else
#error "unsupported platform"
#endif
}

}  // namespace hwinfo