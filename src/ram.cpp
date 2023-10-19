// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <string>

#include <hwinfo/ram.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& RAM::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& RAM::name() const { return _name; }

// _____________________________________________________________________________________________________________________
const std::string& RAM::model() const { return _model; }

// _____________________________________________________________________________________________________________________
const std::string& RAM::serialNumber() const { return _serialNumber; }

// _____________________________________________________________________________________________________________________
int64_t RAM::total_Bytes() const { return _total_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t RAM::frequency_Hz() const { return _frequency_Hz; }

}  // namespace hwinfo