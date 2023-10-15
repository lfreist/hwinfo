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

RAM::RAM() {
  _name = "<unknown>";
  _vendor = "<unknown>";
  _serialNumber = "<unknown>";
  _model = "<unknown>";
  _total_Bytes = -1;
  _free_Bytes = -1;
  _available_Bytes = -1;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE