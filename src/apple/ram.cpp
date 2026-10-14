// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/ram.h>

#include <cstdint>

#include "internal/smbios.h"
#include "internal/sysctl.h"

namespace hwinfo {

result<Memory> memory() {
  return internal::sysctl_value<std::uint64_t>("hw.memsize").transform([](std::uint64_t total) {
    // Modules and slots from SMBIOS on Intel Macs. Apple Silicon has no SMBIOS (memory is part of the SoC package).
    const auto& table = internal::system_table();
    Memory memory = table ? internal::memory_info(*table) : Memory{};
    memory.total = Bytes{total};
    return memory;
  });
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
