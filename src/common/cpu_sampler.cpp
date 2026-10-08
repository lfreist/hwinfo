// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/monitoring.h>

#include <algorithm>

#include "internal/cpu_ticks.h"

namespace hwinfo {

namespace {

double utilization(const detail::CpuTicks& before, const detail::CpuTicks& after) {
  if (after.total <= before.total || after.busy < before.busy) {
    return 0.0;
  }
  const auto busy = static_cast<double>(after.busy - before.busy);
  const auto total = static_cast<double>(after.total - before.total);
  return std::clamp(busy / total, 0.0, 1.0);
}

}  // namespace

CpuSampler::CpuSampler() : _last(internal::read_cpu_ticks().value_or(std::vector<detail::CpuTicks>{})) {}

result<CpuLoad> CpuSampler::sample() {
  auto current = internal::read_cpu_ticks();
  if (!current) {
    return std::unexpected(current.error());
  }
  if (_last.size() != current->size()) {
    // no (compatible) baseline, e.g. a core went offline: measure since boot
    _last.assign(current->size(), {});
  }
  CpuLoad load;
  load.total = utilization(_last.front(), current->front());
  load.per_thread.reserve(current->size() - 1);
  for (std::size_t i = 1; i < current->size(); ++i) {
    load.per_thread.push_back(utilization(_last[i], (*current)[i]));
  }
  _last = std::move(*current);
  return load;
}

}  // namespace hwinfo
