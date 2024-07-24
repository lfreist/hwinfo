// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#ifdef HWINFO_RAM

#include <hwinfo/ram.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::vector<Memory::Module>& Memory::modules() const { return _modules; }

// _____________________________________________________________________________________________________________________
int64_t Memory::total_Bytes() const {
  int64_t sum = 0;
  for (const auto& module : _modules) {
    sum += module.total_Bytes;
  }
  return sum;
}

}  // namespace hwinfo

#endif  // HWINFO_RAM
