// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <hwinfo/WMIwrapper.h>
#include <hwinfo/disk.h>
#include <hwinfo/utils/stringutils.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
  std::vector<const wchar_t*> res{};
  wmi::queryWMI("Win32_DiskDrive", "Manufacturer", res);
  if (res.empty() || res.front() == nullptr) {
    return {};
  }
  for (const auto& v : res) {
    disks.push_back(Disk());
    disks.back()._vendor = utils::wstring_to_std_string(v);
  }
  res.clear();
  wmi::queryWMI("Win32_DiskDrive", "Model", res);
  if (res.empty() || res.front() == nullptr) {
    return {};
  }
  for (int i = 0; i < res.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    disks[i]._model = utils::wstring_to_std_string(res[i]);
  }
  res.clear();
  wmi::queryWMI("Win32_DiskDrive", "SerialNumber", res);
  if (res.empty() || res.front() == nullptr) {
    return {};
  }
  for (int i = 0; i < res.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    std::wstring tmp(res[i]);
    disks[i]._serialNumber = utils::wstring_to_std_string(res[i]);
  }
  std::vector<const wchar_t*> sizes;
  // it will return L"Size" Str
  wmi::queryWMI("Win32_DiskDrive", "Size", sizes);
  if (sizes.empty() || sizes.front() == nullptr) {
    return {};
  }
  for (int i = 0; i < sizes.size(); ++i) {
    if (i >= disks.size()) {
      break;
    }
    disks[i]._size_Bytes = std::stoll(utils::wstring_to_std_string(sizes[i]));
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS