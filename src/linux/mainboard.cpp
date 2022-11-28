// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <fstream>

#include "hwinfo/mainboard.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVendor() {
  std::string manufacturer;
  for (const auto& path : _candidates) {
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
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getName() {
  std::string name;
  for (const auto& path : _candidates) {
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
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVersion() {
  std::string version;
  for (const auto& path : _candidates) {
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
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getSerialNumber() {
  std::string serialNumber;
  for (const auto& path : _candidates) {
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
}

std::vector<std::string> MainBoard::_candidates = {"/sys/devices/virtual/dmi/", "/sys/class/dmi/"};

}  // namespace hwinfo

#endif  // HWINFO_UNIX