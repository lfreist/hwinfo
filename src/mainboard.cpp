// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <fstream>

#include "hwinfo/WMIwrapper.h"

#include "hwinfo/mainboard.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard(const std::string &manufacturer,
                     const std::string &product,
                     const std::string &version,
                     const std::string &serialNumber) {
  _manufacturer = manufacturer;
  _name = product;
  _version = version;
  _serialNumber = serialNumber;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::manufacturer() {
  if (_manufacturer.empty()) {
    _manufacturer = getManufacturer();
  }
  return _manufacturer;
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