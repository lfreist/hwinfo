// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "../platform.h"

#ifdef HWINFO_APPLE

//#include "../mainboard.h"

namespace hwinfo {
namespace mainboard {

// _____________________________________________________________________________________________________________________
std::string getVendor() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getName() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getVersion() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getSerialNumber() {
  // TODO: implement
  return "<unknown>";
}

}  // namespace mainboard

MainBoard::MainBoard() {
  _vendor = "";
  _name = "";
  _version = "";
  _serialNumber = "";
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE