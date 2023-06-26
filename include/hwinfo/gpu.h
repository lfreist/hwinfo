// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

class GPU {
  friend std::vector<GPU> getAllGPUs();

 public:
  ~GPU() = default;

  const std::string& vendor() const;
  const std::string& name() const;
  const std::string& driverVersion() const;
  int64_t memory_Bytes() const;
  int64_t min_frequency_MHz() const;
  int64_t current_frequency_MHz() const;
  int64_t max_frequency_MHz() const;
  int id() const;

 private:
  GPU() = default;
  std::string _vendor{};
  std::string _name{};
  std::string _driverVersion{};
  int64_t _memory_Bytes{0};
  int64_t _maxFrequency_MHz{0};
  int64_t _minFrequency_MHz{0};
  int64_t _currentFrequency_MHz{0};
  int _id{0};
};

std::vector<GPU> getAllGPUs();

}  // namespace hwinfo
