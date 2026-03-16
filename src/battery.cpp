// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/battery.h"

#include <ostream>

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
Battery::Battery(std::uint32_t id) { _id = id; }

// _____________________________________________________________________________________________________________________
std::uint32_t Battery::id() const {
  return _id;
}

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
bool Battery::charging() const { return state() == State::CHARGING; }

// _____________________________________________________________________________________________________________________
bool Battery::discharging() const { return state() == State::DISCHARGING; }

// _____________________________________________________________________________________________________________________
double Battery::capacity() { return static_cast<double>(energyNow()) / energyFull(); }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::ostream& operator<<(std::ostream& os, const Battery& battery) {
  os << "Battery{.id=" << battery._id
     << "', .vendor='" << battery._vendor
     << "', .model='" << battery._model
     << "', .serial_number='" << battery._serialNumber
     << "', technology='" << battery._technology
     << "', .full_capacity=" << battery._energyFull
     << ", .state='" << battery.state() << "'}";
  return os;
}

// _____________________________________________________________________________________________________________________
std::ostream& operator<<(std::ostream& os, const Battery::State& state) {
  switch (state) {
    case Battery::State::CHARGING:
      os << "Battery::State::CHARGING";
      break;
    case Battery::State::DISCHARGING:
      os << "Battery::State::DISCHARGING";
      break;
    case Battery::State::UNKNOWN:
      os << "Battery::State::UNKNOWN";
      break;
  }
  return os;
}

}  // namespace hwinfo
