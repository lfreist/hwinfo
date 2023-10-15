// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "../platform.h"

#ifdef HWINFO_APPLE

#include <sys/sysctl.h>

#include <string>
#include <vector>

//#include "../ram.h"

namespace hwinfo {
namespace ram {

// _____________________________________________________________________________________________________________________
std::string getVendor() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getName() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getModel() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getSerialNumber() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t getTotalSize_Bytes() {
  int64_t memsize = 0;
  size_t size = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0) {
    return memsize;
  }
  return -1;
}

int64_t getAvailableMemory() { return -1; }

}  // namespace ram
}  // namespace hwinfo

#endif  // HWINFO_APPLE