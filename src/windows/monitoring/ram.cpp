// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/monitoring.h>
#include <windows.h>

#include "internal/windows_error.h"

namespace hwinfo {

result<MemoryUsage> memory_usage() {
  MEMORYSTATUSEX status{};
  status.dwLength = sizeof(status);
  if (!GlobalMemoryStatusEx(&status)) {
    return std::unexpected(internal::last_error("GlobalMemoryStatusEx"));
  }
  // Windows does not report unused memory separately: ullAvailPhys covers the free, zeroed and standby (cache) lists.
  return MemoryUsage{
      .total = {status.ullTotalPhys},
      .free = {status.ullAvailPhys},
      .available = {status.ullAvailPhys},
  };
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
