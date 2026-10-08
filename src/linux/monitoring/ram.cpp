// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/monitoring.h>

#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

result<MemoryUsage> memory_usage() {
  return internal::read_file("/proc/meminfo").and_then(internal::procfs::parse_meminfo);
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
