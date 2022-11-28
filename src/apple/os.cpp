// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <sys/sysctl.h>

#include <sstream>
#include <string>

#include "hwinfo/os.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string OS::getFullName() {
  std::string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.os", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "macOS <unknown version>";
}

// _____________________________________________________________________________________________________________________
std::string OS::getName() {
  std::string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.os", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "macOS";
}

// _____________________________________________________________________________________________________________________
std::string OS::getVersion() {
  std::string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.osrelease", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "<unknown version>";
}

// _____________________________________________________________________________________________________________________
std::string OS::getKernel() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
bool OS::getIs64bit() { return true; }

}  // namespace hwinfo

#endif  // HWINFO_APPLE