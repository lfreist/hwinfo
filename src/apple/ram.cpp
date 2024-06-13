// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <hwinfo/ram.h>
#include <sys/sysctl.h>

#include <string>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
int64_t getMemSize() {
  int64_t memSize;
  size_t size = sizeof(memSize);

  if (sysctlbyname("hw.memsize", &memSize, &size, nullptr, 0) == 0) {
    return memSize;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
Memory::Memory() {
  // TODO: get information for actual memory modules (DIMM)
  Module module;
  module.vendor = "<unknown>";
  module.name = "<unknown>";
  module.serial_number = "<unknown>";
  module.model = "<unknown>";
  module.id = 0;
  module.total_Bytes = getMemSize();
  module.frequency_Hz = -1;
  _modules.push_back(module);
}

// _____________________________________________________________________________________________________________________
int64_t Memory::free_Bytes() const {
  // TODO: implement
  return -1;
}

// _____________________________________________________________________________________________________________________
int64_t Memory::available_Bytes() const {
  int64_t usableMemSize;
  size_t size = sizeof(usableMemSize);

  if (sysctlbyname("hw.memsize_usable", &usableMemSize, &size, nullptr, 0) == 0) {
    return usableMemSize;
  }

  return -1;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE