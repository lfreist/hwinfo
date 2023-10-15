// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include "platform.h"

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

class GPU {
  friend std::vector<GPU> getAllGPUs();

 public:
  ~GPU() = default;

  HWI_NODISCARD const std::string& vendor() const { return _vendor; }
  HWI_NODISCARD const std::string& name() const { return _name; }
  HWI_NODISCARD const std::string& driverVersion() const { return _driverVersion; }
  HWI_NODISCARD int64_t memory_Bytes() const { return _memory_Bytes; }
  HWI_NODISCARD int64_t frequency_MHz() const { return _frequency_MHz; }
  HWI_NODISCARD int num_cores() const { return _num_cores; }
  HWI_NODISCARD int id() const { return _id; }

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

#if defined(HWINFO_APPLE)
#include "apple/gpu.h"
#elif defined(HWINFO_LINUX)
#include "linux/gpu.h"
#elif defined(HWINFO_WINDOWS)
#include "windows/gpu.h"
#endif
