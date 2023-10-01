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
namespace ram {

// _____________________________________________________________________________________________________________________
std::string getVendor() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Manufacturer", names);
  if (names.empty()) {
    return "<unknown>";
  }
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return utils::wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string getName() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Name", names);
  if (names.empty()) {
    return "<unknown>";
  }
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return utils::wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string getModel() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "PartNumber", names);
  if (names.empty()) {
    return "<unknown>";
  }
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  return utils::wstring_to_std_string(ret);
}

// _____________________________________________________________________________________________________________________
std::string getSerialNumber() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "SerialNumber", names);
  if (names.empty()) {
    return "<unknown>";
  }
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
int64_t getTotalSize_Bytes() {
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return static_cast<int64_t>(status.ullTotalPhys);
}

// _____________________________________________________________________________________________________________________
int64_t getFreeMemory() {
  // it will return L"FreePhysicalMemory" Str
  std::vector<wchar_t*> memories{};
  // Number of kilobytes of physical memory currently unused and available.
  wmi::queryWMI("CIM_OperatingSystem", "FreePhysicalMemory", memories);
  if (!memories.empty()) {
    if (memories.front() == nullptr) {
      return -1;
    }
    // keep it as totalSize_Bytes
    return std::stoll(utils::wstring_to_std_string(memories[0])) * 1024;
  }
  return -1;
}
}  // namespace ram

// _____________________________________________________________________________________________________________________
RAM::RAM() {
  _name = ram::getName();
  _vendor = ram::getVendor();
  _serialNumber = ram::getSerialNumber();
  _model = ram::getModel();
  _total_Bytes = ram::getTotalSize_Bytes();
  _free_Bytes = ram::getFreeMemory();
  _available_Bytes = ram::getFreeMemory();
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS