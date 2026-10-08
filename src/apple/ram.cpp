// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/ram.h>

#include <cstdint>

#include "internal/sysctl.h"

namespace hwinfo {

result<Memory> memory() {
  // TODO: memory modules (DIMMs) are not reported (Apple Silicon has on-package memory).
  return internal::sysctl_value<std::uint64_t>("hw.memsize").transform([](std::uint64_t total) {
    return Memory{.total = Bytes{total}, .modules = {}};
  });
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
