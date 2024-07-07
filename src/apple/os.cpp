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
  _name = "macOS";

  std::string kernel_name;
  kernel_name.resize(size);
  if (sysctlbyname("kern.ostype", static_cast<void*>(const_cast<char*>(kernel_name.data())), &size, nullptr, 0) == 0) {
    kernel_name.resize(size);  // trim the string to the actual size
    kernel_name.pop_back();    // remove unprintable character at the end
    _kernel = kernel_name;
  } else {
    _kernel = "<unknown name>";
  }

  std::string kernel_version;
  kernel_version.resize(size);
  if (sysctlbyname("kern.osrelease", static_cast<void*>(const_cast<char*>(kernel_version.data())), &size, nullptr, 0) ==
      0) {
    kernel_version.resize(size);
    kernel_version.pop_back();

    _kernel = _kernel + " " + kernel_version;
  } else {
    _kernel = _kernel + " <unknown version>";
  }

  std::string os_version;
  os_version.resize(size);

  if (sysctlbyname("kern.osproductversion", static_cast<void*>(const_cast<char*>(os_version.data())), &size, nullptr,
                   0) == 0) {
    os_version.resize(size);
    os_version.pop_back();
    _version = os_version;
  } else {
    _version = "<unknown>";
  }

  _64bit = true;
  _32bit = !_64bit;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
