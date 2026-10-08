// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/ram.h>

#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

result<Memory> memory() {
  // TODO: memory modules (DIMMs) are only available via SMBIOS type 17 tables, which require root privileges.
  return internal::read_file("/proc/meminfo")
      .and_then(internal::procfs::parse_meminfo)
      .transform([](const MemoryUsage& usage) { return Memory{.total = usage.total, .modules = {}}; });
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
