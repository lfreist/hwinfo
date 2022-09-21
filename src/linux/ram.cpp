// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <string>
#include <vector>

#include <unistd.h>

#include "hwinfo/ram.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string RAM::getVendor() {
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getName() {
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getModel() {
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getSerialNumber() {
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getTotalSize_Bytes() {
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGESIZE);
  if ((pages > 0) && (page_size > 0)) {
    return pages * page_size;
  }
  return -1;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX