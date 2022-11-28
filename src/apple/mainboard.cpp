// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include "hwinfo/mainboard.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVendor() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getName() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getVersion() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string MainBoard::getSerialNumber() {
  // TODO: implement
  return "<unknown>";
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE