// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#pragma once

#include "../platform.h"

#ifdef HWINFO_WINDOWS

#include <string>

#include "../utils/stringutils.h"
#include "utils/wmi_wrapper.h"

namespace hwinfo {
namespace mainboard {

// _____________________________________________________________________________________________________________________
std::string getVendor() {
  std::vector<const wchar_t*> manufacturer{};
  wmi::queryWMI("Win32_BaseBoard", "Manufacturer", manufacturer);
  if (manufacturer.empty()) {
    return "<unknown>";
  }
  auto ret = manufacturer[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return utils::wstring_to_std_string(tmp);
}

// _____________________________________________________________________________________________________________________
std::string getName() {
  std::vector<const wchar_t*> name{};
  wmi::queryWMI("Win32_BaseBoard", "Product", name);
  if (name.empty()) {
    return "<unknown>";
  }
  auto ret = name[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return utils::wstring_to_std_string(tmp);
}

// _____________________________________________________________________________________________________________________
std::string getVersion() {
  std::vector<const wchar_t*> version{};
  wmi::queryWMI("Win32_BaseBoard", "Version", version);
  if (version.empty()) {
    return "<unknown>";
  }
  auto ret = version[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return utils::wstring_to_std_string(tmp);
}

// _____________________________________________________________________________________________________________________
std::string getSerialNumber() {
  std::vector<const wchar_t*> serialNumber{};
  wmi::queryWMI("Win32_BaseBoard", "SerialNumber", serialNumber);
  if (serialNumber.empty()) {
    return "<unknown>";
  }
  auto ret = serialNumber[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return utils::wstring_to_std_string(tmp);
}
}  // namespace mainboard

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard() {
  _vendor = mainboard::getVendor();
  _name = mainboard::getName();
  _version = mainboard::getVersion();
  _serialNumber = mainboard::getSerialNumber();
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS