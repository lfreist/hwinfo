// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/mainboard.h"

#include <fstream>
#include <utility>

#include "hwinfo/WMIwrapper.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string& MainBoard::vendor() { return _vendor; }

// _____________________________________________________________________________________________________________________
std::string& MainBoard::name() { return _name; }

// _____________________________________________________________________________________________________________________
std::string& MainBoard::version() { return _version; }

// _____________________________________________________________________________________________________________________
std::string& MainBoard::serialNumber() { return _serialNumber; }

}  // namespace hwinfo