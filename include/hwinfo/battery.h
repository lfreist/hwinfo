// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>
#include <limits>

namespace hwinfo {

class HWINFO_API Battery {
  friend std::vector<Battery> getAllBatteries();
  friend std::ostream& operator<<(std::ostream& os, const Battery& battery);

 public:
  static constexpr std::uint32_t invalid_id = std::numeric_limits<std::uint32_t>::max();

 public:
  enum class State {
    CHARGING,
    DISCHARGING,
    UNKNOWN
  };

 public:
  explicit Battery(std::uint32_t = 0);
  ~Battery() = default;

  std::string& vendor();
  std::string& model();
  std::string& serialNumber();
  std::string& technology();
  uint32_t energyFull();

  double capacity();

  HWI_NODISCARD std::uint32_t id() const;
  HWI_NODISCARD std::string getVendor() const;
  HWI_NODISCARD std::string getModel() const;
  HWI_NODISCARD std::string getSerialNumber() const;
  HWI_NODISCARD std::string getTechnology() const;
  HWI_NODISCARD uint32_t getEnergyFull() const;

  HWI_NODISCARD uint32_t energyNow() const;
  HWI_NODISCARD bool charging() const;
  HWI_NODISCARD bool discharging() const;
  HWI_NODISCARD State state() const;

 private:
  std::uint32_t _id = invalid_id;
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  std::string _technology;
  uint32_t _energyFull = 0;
};

std::vector<Battery> getAllBatteries();

std::ostream& operator<<(std::ostream& os, const Battery::State& state);
std::ostream& operator<<(std::ostream& os, const Battery& battery);

}  // namespace hwinfo
