// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(HWINFO_WINDOWS)
#include <windows.h>

#include "hwinfo/WMIwrapper.h"
#endif

#include <cstdint>
#include <string>

namespace hwinfo {

class RAM {
 public:
  RAM();
  ~RAM() = default;

  const std::string& vendor() const { return _vendor; }
  const std::string& name() const { return _name; }
  const std::string& model() const { return _model; }
  const std::string& serialNumber() const { return _serialNumber; }
  int64_t total_Bytes() const { return _total_Bytes; }
  int64_t free_Bytes() const { return _free_Bytes; }
  int64_t available_Bytes() const { return _available_Bytes; }

 private:
  std::string _vendor{};
  std::string _name{};
  std::string _model{};
  std::string _serialNumber{};
  int64_t _total_Bytes = -1;
  int64_t _free_Bytes = -1;
  int64_t _available_Bytes = -1;
  int _frequency = -1;
};

}  // namespace hwinfo

#if defined(HWINFO_APPLE)
#include "apple/ram.h"
#elif defined(HWINFO_LINUX)
#include "linux/ram.h"
#elif defined(HWINFO_WINDOWS)
#include "windows/ram.h"
#endif

