// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <string>
#include <vector>

namespace hwinfo {

class MainBoard {
  friend std::string get_dmi_by_name(const std::string& name);

 public:
  MainBoard();
  ~MainBoard() = default;

  std::string vendor() const;
  std::string name() const;
  std::string version() const;
  std::string serialNumber() const;

 private:
  std::string _vendor;
  std::string _name;
  std::string _version;
  std::string _serialNumber;
};

}  // namespace hwinfo