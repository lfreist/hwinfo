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
Disk::Disk(const std::string &vendor, const std::string &model, const std::string &serialNumber, int64_t size_Bytes) {
  _vendor = vendor;
  _model = model;
  _serialNumber = serialNumber;
  _size_Bytes = size_Bytes;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::vendor() const {
  return _vendor;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::model() const {
  return _model;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::serialNumber() const {
  return _serialNumber;
}

// _____________________________________________________________________________________________________________________
int64_t Disk::size_Bytes() const {
  return _size_Bytes;
}


// =====================================================================================================================
struct TmpDisk {
  std::string vendor;
  std::string model;
  std::string serialNumber;
  int64_t sizeBytes;
};

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