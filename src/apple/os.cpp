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
  size_t size = 1024;
  std::string os_name;
  os_name.resize(size);
  if (sysctlbyname("kern.os", (void*)(os_name.data()), &size, nullptr, 0) == 0) {
    os_name.resize(size);  // trim the string to the actual size
    return os_name;
  }
  return "macOS <unknown version>";
}

// _____________________________________________________________________________________________________________________
std::string OS::getName() {
  size_t size = 1024;
  std::string os_name;
  os_name.resize(size);
  if (sysctlbyname("kern.os", (void*)(os_name.data()), &size, nullptr, 0) == 0) {
    os_name.resize(size);  // trim the string to the actual size
    return os_name;
  }
  return "macOS";
}

// _____________________________________________________________________________________________________________________
std::string OS::getVersion() {
  size_t size = 1024;
  std::string os_version;
  os_version.resize(size);
  if (sysctlbyname("kern.osrelease", (void*)(os_version.data()), &size, nullptr, 0) == 0) {
    os_version.resize(size);  // trim the string to the actual size
    return os_version;
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
