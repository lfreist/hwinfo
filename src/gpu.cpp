// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/gpu.h>

#include <string>

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