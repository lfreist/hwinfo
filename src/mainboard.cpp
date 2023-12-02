// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/mainboard.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string MainBoard::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
std::string MainBoard::name() const { return _name; }

// _____________________________________________________________________________________________________________________
std::string MainBoard::version() const { return _version; }

// _____________________________________________________________________________________________________________________
std::string MainBoard::serialNumber() const { return _serialNumber; }

}  // namespace hwinfo