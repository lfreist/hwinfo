// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#include <vector>

namespace hwinfo {

class MainBoard {
 public:
  MainBoard() = default;
  MainBoard(const std::string &manufacturer,
            const std::string &product,
            const std::string &version,
            const std::string &serialNumber);
  ~MainBoard() = default;

  std::string &manufacturer();
  std::string &name();
  std::string &version();
  std::string &serialNumber();

  static std::string getManufacturer();
  static std::string getName();
  static std::string getVersion();
  static std::string getSerialNumber();

 private:
  std::string _manufacturer;
  std::string _name;
  std::string _version;
  std::string _serialNumber;

#if defined(unix) || defined(__unix) || defined(__unix__)
  static std::vector<std::string> _candidates;
#endif
};

}  // namespace hwinfo