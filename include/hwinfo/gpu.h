// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

class GPU {
 public:
  GPU() = default;
  GPU(const std::string& vendor, const std::string& name, const std::string& driverVersion, int64_t memory_Bytes);
  ~GPU() = default;

  std::string& vendor();
  std::string& name();
  std::string& driverVersion();
  int64_t memory_Bytes();

  static std::string getVendor();
  static std::string getName();
  static std::string getDriverVersion();
  static int64_t getMemory_Bytes();

 private:
  std::string _vendor;
  std::string _name;
  std::string _driverVersion;
  int64_t _memory_Bytes = -1;
};

}  // namespace hwinfo
