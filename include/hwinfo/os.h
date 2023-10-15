// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>

namespace hwinfo {

class OS {
 public:
  OS() {
    _32bit = getIs32bit();
    _64bit = getIs64bit();
    _bigEndian = getIsBigEndian();
    _littleEndian = !_bigEndian;
  }
  ~OS() = default;

  std::string fullName() {
    if (_fullName.empty()) {
      _fullName = getFullName();
    }
    return _fullName;
  }

  std::string name() {
    if (_name.empty()) {
      _name = getName();
    }
    return _name;
  }

  std::string version() {
    if (_version.empty()) {
      _version = getVersion();
    }
    return _version;
  }

  std::string kernel() {
    if (_kernel.empty()) {
      _kernel = getKernel();
    }
    return _kernel;
  }


  [[nodiscard]] bool is32bit() const { return _32bit; }
  [[nodiscard]] bool is64bit() const { return _64bit; }
  [[nodiscard]] bool isBigEndian() const { return _bigEndian; }
  [[nodiscard]] bool isLittleEndian() const { return _littleEndian; }

  static std::string getFullName();
  static std::string getName();
  static std::string getVersion();
  static std::string getKernel();
  static bool getIs32bit() { return !getIs64bit(); }
  static bool getIs64bit();
  static bool getIsBigEndian() {
    char16_t dummy = 0x0102;
    return ((char*)&dummy)[0] == 0x01;
  }

  static bool getIsLittleEndian() {
    char16_t dummy = 0x0102;
    return ((char*)&dummy)[0] == 0x02;
  }

 private:
  std::string _fullName;
  std::string _name;
  std::string _version;
  std::string _kernel;
  bool _32bit = false;
  bool _64bit = false;
  bool _bigEndian = false;
  bool _littleEndian = false;
};

}  // namespace hwinfo

#if defined(HWINFO_APPLE)
#include "apple/os.h"
#elif defined(HWINFO_LINUX)
#include "linux/os.h"
#elif defined(HWINFO_WINDOWS)
#include "windows/os.h"
#endif

