// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <fstream>

#include "hwinfo/WMIwrapper.h"

#include "hwinfo/mainboard.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
MainBoard::MainBoard(const std::string &manufacturer,
                     const std::string &product,
                     const std::string &version,
                     const std::string &serialNumber) {
  _manufacturer = manufacturer;
  _name = product;
  _version = version;
  _serialNumber = serialNumber;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::manufacturer() {
  if (_manufacturer.empty()) {
    _manufacturer = getManufacturer();
  }
  return _manufacturer;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::version() {
  if (_version.empty()) {
    _version = getVersion();
  }
  return _version;
}

// _____________________________________________________________________________________________________________________
std::string& MainBoard::serialNumber() {
  if (_serialNumber.empty()) {
    _serialNumber = getSerialNumber();
  }
  return _serialNumber;
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getManufacturer() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string manufacturer;
  for (const auto& path: _candidates) {
    std::string full_path = path + "id/board_vendor";
    std::ifstream f(full_path);
    if (f) {
      getline(f, manufacturer);
      if (!manufacturer.empty()) {
        return manufacturer;
      }
    }
  }
  return "<unknown>";
#elif defined(__APPLE__)
  return "<unknown>"
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> manufacturer{};
  wmi::queryWMI("Win32_BaseBoard", "Manufacturer", manufacturer);
  auto ret = manufacturer[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getName() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string name;
  for (const auto& path: _candidates) {
    std::string full_path = path + "id/board_name";
    std::ifstream f(full_path);
    if (f) {
      getline(f, name);
      if (!name.empty()) {
        return name;
      }
    }
  }
  return "<unknown>";
#elif defined(__APPLE__)
  return "<unknown>"
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> name{};
  wmi::queryWMI("Win32_BaseBoard", "Product", name);
  auto ret = name[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVersion() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string version;
  for (const auto& path: _candidates) {
    std::string full_path = path + "id/board_version";
    std::ifstream f(full_path);
    if (f) {
      getline(f, version);
      if (!version.empty()) {
        return version;
      }
    }
  }
  return "<unknown>";
#elif defined(__APPLE__)
  return "<unknown>"
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> version{};
  wmi::queryWMI("Win32_BaseBoard", "Version", version);
  auto ret = version[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getSerialNumber() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string serialNumber;
  for (const auto& path: _candidates) {
    std::string full_path = path + "id/board_serial";
    std::ifstream f(full_path);
    if (f) {
      getline(f, serialNumber);
      if (serialNumber.empty()) {
        return serialNumber;
      }
    }
  }
  return "<unknown>";
#elif defined(__APPLE__)
  return "<unknown>"
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> serialNumber{};
  wmi::queryWMI("Win32_BaseBoard", "SerialNumber", serialNumber);
  auto ret = serialNumber[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

#if defined(unix) || defined(__unix) || defined(__unix__)
std::vector<std::string> MainBoard::_candidates = {"/sys/devices/virtual/dmi/", "/sys/class/dmi/"};
#endif

}  // namespace hwinfo