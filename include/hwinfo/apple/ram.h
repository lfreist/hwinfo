// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <sys/sysctl.h>

#include <string>
#include <vector>

#include "hwinfo/ram.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string RAM::getVendor() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getName() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getModel() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string RAM::getSerialNumber() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getTotalSize_Bytes() {
  int64_t memsize = 0;
  size_t size = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0) {
    return memsize;
  }
  return -1;
}

int64_t RAM::getAvailableMemory() { return -1; }

}  // namespace hwinfo

#endif  // HWINFO_APPLE