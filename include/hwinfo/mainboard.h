// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

#include <hwinfo/platform.h>

namespace hwinfo {

class MainBoard {
  friend std::string get_dmi_by_name(const std::string& name);

 public:
  MainBoard();
  ~MainBoard() = default;

  std::string& vendor();
  std::string& name();
  std::string& version();
  std::string& serialNumber();

 private:
  std::string _vendor;
  std::string _name;
  std::string _version;
  std::string _serialNumber;
};

}  // namespace hwinfo