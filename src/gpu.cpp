// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")
#endif

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
GPU::GPU(const std::string &name, const std::string &driverVersion, int64_t memory_Bytes) {
  _name = name;
  _driverVersion = driverVersion;
  _memory_Bytes = memory_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string &GPU::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string &GPU::driverVersion() {
  if (_driverVersion.empty()) {
    _driverVersion = getDriverVersion();
  }
  return _driverVersion;
}

// _____________________________________________________________________________________________________________________
int64_t GPU::memory_Bytes() {
  if (_memory_Bytes == -1) {
    _memory_Bytes = getMemory_Bytes();
  }
  return _memory_Bytes;
}

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
  return {tmp.begin(), tmp.end()};
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
  return {tmp.begin(), tmp.end()};
#else
#error "unsupported platform"
#endif
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemory_Bytes() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return -1;
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<unsigned long long> memory{};
  wmi::queryWMI("WIN32_VideoController", "AdapterRam", memory);
  return static_cast<int64_t>(memory[0] * 2);
#else
#error "unsupported platform"
#endif
}

}  // namespace hwinfo