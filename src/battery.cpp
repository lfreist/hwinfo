// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/battery.h"

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
Battery::Battery(int8_t id) { _id = id; }

// _____________________________________________________________________________________________________________________
std::string& Battery::vendor() {
  if (_vendor.empty()) {
    _vendor = getVendor();
  }
  return _vendor;
}

// _____________________________________________________________________________________________________________________
std::string& Battery::model() {
  if (_model.empty()) {
    _model = getModel();
  }
  return _model;
}

// _____________________________________________________________________________________________________________________
std::string& Battery::serialNumber() {
  if (_serialNumber.empty()) {
    _serialNumber = getSerialNumber();
  }
  return _serialNumber;
}

// _____________________________________________________________________________________________________________________
std::string& Battery::technology() {
  if (_technology.empty()) {
    _technology = getTechnology();
  }
  return _technology;
}

// _____________________________________________________________________________________________________________________
uint32_t Battery::energyFull() {
  if (_energyFull == 0) {
    _energyFull = getEnergyFull();
  }
  return _energyFull;
}

// _____________________________________________________________________________________________________________________
double Battery::capacity() { return static_cast<double>(energyNow()) / energyFull(); }

}  // namespace hwinfo