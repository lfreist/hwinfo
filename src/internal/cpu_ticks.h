// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/monitoring.h>

#include <vector>

namespace hwinfo::internal {

// Platform specific: cumulative CPU tick counters, [0]: all cores, [1 + i]: logical core i.
result<std::vector<detail::CpuTicks>> read_cpu_ticks();

}  // namespace hwinfo::internal
