// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/mainboard.h"

#include <fstream>
#include <utility>

#include "hwinfo/WMIwrapper.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard(std::string vendor, std::string product, std::string version, std::string serialNumber)
    : _vendor(std::move(vendor)),
      _name(std::move(product)),
      _version(std::move(version)),
      _serialNumber(std::move(serialNumber)) {}

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