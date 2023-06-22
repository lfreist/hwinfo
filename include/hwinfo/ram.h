// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>

namespace hwinfo {

class RAM {
 public:
  RAM() = default;
  RAM(std::string& vendor, std::string& name, std::string& model, std::string& serialNumber, int64_t size_Bytes);
  ~RAM() = default;

  std::string& vendor();
  std::string& name();
  std::string& model();
  std::string& serialNumber();
  int64_t totalSize_Bytes();
  int64_t availableMemory();

  static std::string getVendor();
  static std::string getName();
  static std::string getModel();
  static std::string getSerialNumber();
  static int64_t getTotalSize_Bytes();
  static int64_t getAvailableMemory();

 private:
  std::string _vendor;
  std::string _name;
  std::string _model;
  std::string _serialNumber;
  int64_t _totalSize_Bytes = -1;
  int64_t _totalFreeSize_Bytes = -1;
  int _clockSpeed = -1;
};

}  // namespace hwinfo
