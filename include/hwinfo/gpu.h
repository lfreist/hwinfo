// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

#ifdef USE_OCL
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <missocl/opencl.h>
#endif

namespace hwinfo {

class GPU {
  friend std::vector<GPU> getAllGPUs();

 public:
  ~GPU() = default;

  const std::string& vendor() const;
  const std::string& name() const;
  const std::string& driverVersion() const;
  int64_t memory_Bytes() const;
  int64_t frequency_MHz() const;
  int num_cores() const;
  int id() const;

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
