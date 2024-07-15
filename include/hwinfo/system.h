// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#if defined(HWINFO_CPU) && defined(HWINFO_GPU) && defined(HWINFO_RAM) && defined(HWINFO_DISK)
#define HWINFO_SYSTEM

#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/gpu.h>
#include <hwinfo/ram.h>

#include <vector>

namespace hwinfo {

class System {
 public:
  System() = default;

  std::vector<CPU>& CPUs() const;
  std::vector<GPU>& GPUs() const;
  std::vector<Memory>& RAMs() const;
  std::vector<Disk>& Disks() const;

 private:
  std::vector<CPU> _cpuSockets;
  std::vector<GPU> _gpus;
  std::vector<Memory> _ramBars;
  std::vector<Disk> _disks;
};

}  // namespace hwinfo

#endif  // HWINFO_SYSTEM
