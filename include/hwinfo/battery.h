// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <optional>

#include "hwinfo/platform.h"

namespace hwinfo {

class Battery {
 public:
  Battery(uint8_t id = 0);
  ~Battery() = default;

  std::string &vendor();
  std::string &model();
  std::string &serialNumber();
  std::string &technology();
  uint32_t energyFull();

  double capacity();

  std::string getVendor();
  std::string getModel();
  std::string getSerialNumber();
  std::string getTechnology();
  uint32_t getEnergyFull();

  uint32_t energyNow();
  bool charging();
  bool discharging();

 private:
  uint8_t _id = 0;
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  std::string _technology;
  uint32_t _energyFull = 0;
};

std::optional<Battery> getAllBatteries() {
  // TODO: implement
  return {};
}

}  // namespace hwinfo

