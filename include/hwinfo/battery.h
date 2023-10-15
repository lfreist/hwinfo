// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hwinfo/platform.h"

namespace hwinfo {

class Battery {
  friend std::vector<Battery> getAllBatteries();

 public:
  explicit Battery(int8_t id = 0) { _id = id; }
  ~Battery() = default;

  std::string& vendor() {
    if (_vendor.empty()) {
      _vendor = getVendor();
    }
    return _vendor;
  }

  std::string& model() {
    if (_model.empty()) {
      _model = getModel();
    }
    return _model;
  }

  std::string& serialNumber()  {
    if (_serialNumber.empty()) {
      _serialNumber = getSerialNumber();
    }
    return _serialNumber;
  }

  std::string& technology() {
    if (_technology.empty()) {
      _technology = getTechnology();
    }
    return _technology;
  }

  uint32_t energyFull() {
    if (_energyFull == 0) {
      _energyFull = getEnergyFull();
    }
    return _energyFull;
  }

  double capacity() { return static_cast<double>(energyNow()) / energyFull(); }

  [[nodiscard]] std::string getVendor() const;
  [[nodiscard]] std::string getModel() const;
  [[nodiscard]] std::string getSerialNumber() const;
  [[nodiscard]] std::string getTechnology() const;
  [[nodiscard]] uint32_t getEnergyFull() const;

  [[nodiscard]] uint32_t energyNow() const;
  [[nodiscard]] bool charging() const;
  [[nodiscard]] bool discharging() const;

 private:
  int8_t _id = -1;
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  std::string _technology;
  uint32_t _energyFull = 0;
};

std::vector<Battery> getAllBatteries();

}  // namespace hwinfo

#if defined(HWINFO_APPLE)
#include "apple/battery.h"
#elif defined(HWINFO_LINUX)
#include "linux/battery.h"
#elif defined(HWINFO_WINDOWS)
#include "windows/battery.h"
#endif