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
OS::OS() {
  size_t size = 1024;

  std::string os_name;
  os_name.resize(size);
  if (sysctlbyname("kern.ostype", os_name.data(), &size, nullptr, 0) == 0) {
    os_name.resize(size);  // trim the string to the actual size
    _name = os_name;
  } else {
    _name = "macOS";
  }

  std::string os_version;
  os_version.resize(size);
  if (sysctlbyname("kern.osrelease", os_version.data(), &size, nullptr, 0) == 0) {
    os_version.resize(size);  // trim the string to the actual size
    _version = os_version;
  } else {
    _version = "<unknown>";
  }

  _kernel = "<unknown>";

  _64bit = true;
  _32bit = !_64bit;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
