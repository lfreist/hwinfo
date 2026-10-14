// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/ram.h>

#include "internal/file.h"
#include "internal/procfs.h"
#include "internal/smbios.h"

namespace hwinfo {

result<Memory> memory() {
  return internal::read_file("/proc/meminfo")
      .and_then(internal::procfs::parse_meminfo)
      .transform([](const MemoryUsage& usage) {
        // Modules and slots are only in the raw SMBIOS table, which the kernel exposes to root only. Without it (or
        // without SMBIOS, e.g. on most ARM devices) they stay empty.
        const auto& table = internal::system_table();
        Memory memory = table ? internal::memory_info(*table) : Memory{};
        memory.total = usage.total;
        return memory;
      });
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
