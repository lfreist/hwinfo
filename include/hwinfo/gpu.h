// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

class GPU {
 public:
  GPU() = default;
  GPU(const std::string& name, const std::string& driverVersion, int64_t memory_Bytes);
  ~GPU() = default;

  std::string& name();
  std::string& driverVersion();
  int64_t memory_Bytes();
  int xResolution();
  int yResolution();
  int refreshRate();

  static std::string getName();
  static std::string getDriverVersion();
  static int64_t getMemory_Bytes();
  static int getXResolution();
  static int getYResolution();
  static int getRefreshRate();

 private:
  std::string _name;
  std::string _driverVersion;
  int64_t _memory_Bytes = -1;
  int _xResolution = -1;
  int _yResolution = -1;
  int _refreshRate = -1;
};

}  // namespace hwinfo
