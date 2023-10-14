// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

class GPU {
  friend std::vector<GPU> getAllGPUs();

 public:
  ~GPU() = default;

  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD const std::string& name() const;
  HWI_NODISCARD const std::string& driverVersion() const;
  HWI_NODISCARD int64_t memory_Bytes() const;
  HWI_NODISCARD int64_t frequency_MHz() const;
  HWI_NODISCARD int num_cores() const;
  HWI_NODISCARD int id() const;

 private:
  GPU() = default;
  std::string _vendor{};
  std::string _name{};
  std::string _driverVersion{};
  int64_t _memory_Bytes{0};
  int64_t _frequency_MHz{0};
  int _num_cores{0};
  int _id{0};

  std::string _vendor_id{};
  std::string _device_id{};
};

std::vector<GPU> getAllGPUs();
}  // namespace hwinfo
