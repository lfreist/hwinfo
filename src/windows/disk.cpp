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
  std::vector<const wchar_t*> res {};
  wmi::queryWMI("Win32_DiskDrive", "Manufacturer", res);
  if (res.empty()) { return {}; }
  for (const auto &v: res) {
    std::wstring tmp(v);
    disks.push_back(Disk());
    disks.back()._vendor = { tmp.begin(), tmp.end() };
  }
  res.clear();
  wmi::queryWMI("Win32_DiskDrive", "Model", res);
  for (int i = 0; i < res.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    std::wstring tmp(res[i]);
    disks[i]._model = { tmp.begin(), tmp.end() };
  }
  res.clear();
  wmi::queryWMI("Win32_DiskDrive", "SerialNumber", res);
  for (int i = 0; i < res.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    std::wstring tmp(res[i]);
    disks[i]._serialNumber = { tmp.begin(), tmp.end() };
  }
  std::vector<uint64_t> res2;
  // this returns a random same number for all disks...
  wmi::queryWMI("Win32_DiskDrive", "Size", res2);
  for (int i = 0; i < res.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    // TODO: fix this error
    // disks[i]._size_Bytes = res2[i];
    disks[i]._size_Bytes = -1;
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS