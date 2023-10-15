// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE
#ifndef USE_OCL

#include <regex>
#include <string>
#include <vector>

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string GPU::getVendor() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string GPU::getName() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string GPU::getDriverVersion() {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemory_Bytes() {
  // TODO: implement
  return -1;
}

}  // namespace hwinfo

#endif  // USE_OCL
#endif  // HWINFO_APPLE