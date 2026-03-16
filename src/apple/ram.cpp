// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <hwinfo/ram.h>
#include <mach/mach_host.h>
#include <mach/mach_init.h>
#include <mach/vm_statistics.h>
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
uint64_t Memory::total_Bytes() const {
  uint64_t sum = 0;
  for (const auto& module : _modules) {
    sum += module.total_Bytes;
  }
  return sum;
}

// _____________________________________________________________________________________________________________________
int64_t Memory::free_Bytes() const {
  vm_statistics64_data_t vmStats;
  mach_msg_type_number_t infoCount = HOST_VM_INFO64_COUNT;
  kern_return_t kernReturn =
      host_statistics64(mach_host_self(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vmStats), &infoCount);

  if (kernReturn == KERN_SUCCESS) {
    vm_size_t pageSize;
    host_page_size(mach_host_self(), &pageSize);

    int64_t totalMemory = getMemSize();
    if (totalMemory == -1) return -1;

    const int64_t appMemory =
        static_cast<int64_t>(vmStats.internal_page_count - vmStats.purgeable_count) * static_cast<int64_t>(pageSize);
    const int64_t wiredMemory = static_cast<int64_t>(vmStats.wire_count) * static_cast<int64_t>(pageSize);
    const int64_t compressedMemory =
        static_cast<int64_t>(vmStats.compressor_page_count) * static_cast<int64_t>(pageSize);

    const int64_t usedMemory = appMemory + wiredMemory + compressedMemory;
    return totalMemory - usedMemory;
  }

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