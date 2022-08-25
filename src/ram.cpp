// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <string>
#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <Windows.h>
#include "hwinfo/WMIwrapper.h"
#endif
#include "hwinfo/ram.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
RAM::RAM(std::string &manufacturer,
         std::string &name,
         std::string &model,
         std::string &serialNumber,
         int64_t size_Bytes) {
  _manufacturer = manufacturer;
  _name = name;
  _model = model;
  _serialNumber = serialNumber;
  _totalSize_Bytes = size_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string &RAM::manufacturer() {
  if (_manufacturer.empty()) {
    _manufacturer = getManufacturer();
  }
  return _manufacturer;
}

// _____________________________________________________________________________________________________________________
std::string &RAM::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string &RAM::model() {
  if (_model.empty()) {
    _model = getModel();
  }
  return _model;
}

// _____________________________________________________________________________________________________________________
std::string &RAM::serialNumber() {
  if (_serialNumber.empty()) {
    _serialNumber = getSerialNumber();
  }
  return _serialNumber;
}

// _____________________________________________________________________________________________________________________
int64_t RAM::totalSize_Bytes() {
  if (_totalSize_Bytes == -1) {
    _totalSize_Bytes = getTotalSize_Bytes();
  }
  return _totalSize_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string RAM::getManufacturer() {
#if defined(_WIN32) || defined(_WIN64)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Manufacturer", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string RAM::getName() {
#if defined(_WIN32) || defined(_WIN64)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Name", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string RAM::getModel() {
#if defined(_WIN32) || defined(_WIN64)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "PartNumber", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string RAM::getSerialNumber() {
#if defined(_WIN32) || defined(_WIN64)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "SerialNumber", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getTotalSize_Bytes() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  long pages = sysconf(_SC_PHYS_PAGES);
  long page_size = sysconf(_SC_PAGESIZE);
  if ((pages > 0) && (page_size > 0)) {
    return pages * page_size;
  }
  return -1;
#elif defined(__APPLE__)
  int64_t memsize = 0;
  size_t size = sizeof(memsize);
  if (sysctlbyname("hw.memsize", &memsize, &size, nullptr, 0) == 0) {
    return memsize;
  }
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  GlobalMemoryStatusEx(&status);
  return static_cast<int64_t>(status.ullTotalPhys);
#else
  return -1;
#endif
}

// _____________________________________________________________________________________________________________________


// _____________________________________________________________________________________________________________________
}  // namespace hwinfo