// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include "cpu.h"
#include "disk.h"
#include "gpu.h"
#include "ram.h"

#include <string>
#include <vector>

namespace hwinfo {

class System {
 public:
  System() = default;

  std::vector<CPU>& CPUs() const;
  std::vector<GPU>& GPUs() const;
  std::vector<RAM>& RAMs() const;
  std::vector<Disk>& Disks() const;

 private:
  std::vector<CPU> _cpuSockets;
  std::vector<GPU> _gpus;
  std::vector<RAM> _ramBars;
  std::vector<Disk> _disks;
};

}  // namespace hwinfo
