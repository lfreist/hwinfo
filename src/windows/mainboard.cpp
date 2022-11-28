// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <string>

#include "hwinfo/WMIwrapper.h"
#include "hwinfo/mainboard.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVendor() {
  std::vector<const wchar_t*> manufacturer{};
  wmi::queryWMI("Win32_BaseBoard", "Manufacturer", manufacturer);
  auto ret = manufacturer[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getName() {
  std::vector<const wchar_t*> name{};
  wmi::queryWMI("Win32_BaseBoard", "Product", name);
  auto ret = name[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVersion() {
  std::vector<const wchar_t*> version{};
  wmi::queryWMI("Win32_BaseBoard", "Version", version);
  auto ret = version[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getSerialNumber() {
  std::vector<const wchar_t*> serialNumber{};
  wmi::queryWMI("Win32_BaseBoard", "SerialNumber", serialNumber);
  auto ret = serialNumber[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS