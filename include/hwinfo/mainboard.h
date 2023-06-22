// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

#include "hwinfo/platform.h"

namespace hwinfo {

class MainBoard {
 public:
  MainBoard() = default;
  MainBoard(std::string vendor, std::string product, std::string version, std::string serialNumber);
  ~MainBoard() = default;

  std::string& vendor();
  std::string& name();
  std::string& version();
  std::string& serialNumber();

  static std::string getVendor();
  static std::string getName();
  static std::string getVersion();
  static std::string getSerialNumber();

 private:
  std::string _vendor;
  std::string _name;
  std::string _version;
  std::string _serialNumber;

#ifdef HWINFO_UNIX
  static std::vector<std::string> _candidates;
#endif
};

}  // namespace hwinfo