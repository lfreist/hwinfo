// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <fstream>

#include "hwinfo/WMIwrapper.h"

#include "hwinfo/mainboard.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard(const std::string &vendor,
                     const std::string &product,
                     const std::string &version,
                     const std::string &serialNumber)
  : _vendor(vendor), _name(product), _version(version), _serialNumber(serialNumber) {
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::vendor() {
  if (_vendor.empty()) {
    _vendor = getVendor();
  }
  return _vendor;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::version() {
  if (_version.empty()) {
    _version = getVersion();
  }
  return _version;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::serialNumber() {
  if (_serialNumber.empty()) {
    _serialNumber = getSerialNumber();
  }
  return _serialNumber;
}

}  // namespace hwinfo