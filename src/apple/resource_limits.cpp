// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/resource_limits.h>

#include <cstdint>
#include <numeric>
#include <vector>

#include "internal/sysctl.h"

namespace hwinfo {

// macOS has neither CPU quotas, memory limits nor CPU affinity for processes: every logical core is allowed.
result<ResourceLimits> resource_limits() {
  ResourceLimits limits;
  if (const auto logical = internal::sysctl_number<std::uint32_t>("hw.logicalcpu")) {
    limits.allowed_cpus.resize(*logical);
    std::iota(limits.allowed_cpus.begin(), limits.allowed_cpus.end(), std::uint32_t{0});
  }
  return limits;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
