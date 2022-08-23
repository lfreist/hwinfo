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
RAM::RAM() {
  _manufacturer = getManufacturer();
  _name = getName();
  _model = getModel();
  _serialNumber = getSerialNumber();
  _totalSizeBytes = getTotalSizeMiB();
}

// _____________________________________________________________________________________________________________________
RAM::~RAM() = default;

// _____________________________________________________________________________________________________________________
std::string RAM::getManufacturer() {
#if defined(_WIN32) || defined(_WIN64)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_PhysicalMemory", "Manufacturer", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return std::string(tmp.begin(), tmp.end());
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
  return std::string(tmp.begin(), tmp.end());
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
  return std::string(tmp.begin(), tmp.end());
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
  return std::string(tmp.begin(), tmp.end());
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
int64_t RAM::getTotalSizeMiB() {
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
  return status.ullTotalPhys;
#else
#error Unsupported Platform
#endif
}

// _____________________________________________________________________________________________________________________


// _____________________________________________________________________________________________________________________
}  // namespace hwinfo