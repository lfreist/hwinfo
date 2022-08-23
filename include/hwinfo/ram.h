// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>

#ifndef HWINFO_RAM_H_
#define HWINFO_RAM_H_

namespace hwinfo {

class RAM {
 public:
  RAM();
  ~RAM();


  std::string manufacturer() { return _manufacturer; }
  std::string name() { return _name; }
  std::string model() { return _model; }
  std::string serialNumber() { return _serialNumber; }
  [[nodiscard]] int64_t totalSizeBytes() const { return _totalSizeBytes; }

  static std::string getManufacturer();
  static std::string getName();
  static std::string getModel();
  static std::string getSerialNumber();
  static int64_t getTotalSizeMiB();

 private:
  std::string _manufacturer;
  std::string _name;
  std::string _model;
  std::string _serialNumber;
  int64_t _totalSizeBytes = -1;
  int _clockSpeed = -1;
};

}  // namespace hwinfo

#endif //HWINFO_RAM_H_
