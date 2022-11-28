// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <regex>
#include <string>
#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include "hwinfo/utils/subprocess.h"
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")
#endif

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
GPU::GPU(const std::string& vendor, const std::string& name, const std::string& driverVersion, int64_t memory_Bytes)
    : _vendor(vendor), _name(name), _driverVersion(driverVersion) {
  _memory_Bytes = memory_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string& GPU::vendor() {
  if (_vendor.empty()) {
    _vendor = getVendor();
  }
  return _vendor;
}

// _____________________________________________________________________________________________________________________
std::string& GPU::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string& GPU::driverVersion() {
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

}  // namespace hwinfo