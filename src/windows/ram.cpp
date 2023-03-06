// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <Windows.h>

#include <string>
#include <vector>

#include "hwinfo/WMIwrapper.h"
#include "hwinfo/ram.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string RAM::getVendor() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Manufacturer", names);
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string RAM::getName() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Name", names);
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string RAM::getModel() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "PartNumber", names);
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string RAM::getSerialNumber() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "SerialNumber", names);
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getTotalSize_Bytes() {
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return static_cast<int64_t>(status.ullTotalPhys);
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getAvailableMemory() {
  // it will return L"FreePhysicalMemory" Str
  std::vector<wchar_t*> memories{};
  // Number of kilobytes of physical memory currently unused and available.
  wmi::queryWMI("CIM_OperatingSystem", "FreePhysicalMemory", memories);
  if (memories.size() > 0) {
    // keep it as totalSize_Bytes
    return std::stoll(wstring_to_std_string(memories[0])) * 1024;
  }
  return -1;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS