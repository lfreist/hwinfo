// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/ram.h>

#include <vector>

#include "internal/file.h"
#include "internal/procfs.h"
#include "internal/smbios.h"

namespace hwinfo {

result<Memory> memory() {
  return internal::read_file("/proc/meminfo")
      .and_then(internal::procfs::parse_meminfo)
      .transform([](const MemoryUsage& usage) {
        // Modules are only in the raw SMBIOS table, which the kernel exposes to root only. Without it (or without
        // SMBIOS, e.g. on most ARM devices) the module list stays empty.
        const auto table = smbios::read_system_table();
        return Memory{.total = usage.total,
                      .modules = table ? internal::memory_modules(*table) : std::vector<MemoryModule>{}};
      });
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
