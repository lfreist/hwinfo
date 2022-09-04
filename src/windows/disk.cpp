// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <filesystem>

#include "hwinfo/disk.h"
#include "hwinfo/utils/stringutils.h"
#include "hwinfo/WMIwrapper.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
  std::vector<const wchar_t*> vendor {};
  wmi::queryWMI("Win32_Processor", "Name", vendor);
  if (vendor.empty()) { return {}; }
  std::wstring tmp(vendor[0]);
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS