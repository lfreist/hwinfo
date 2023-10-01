// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <string>
#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(_WIN32) || defined(_WIN64)
#include <Windows.h>

#include "hwinfo/WMIwrapper.h"
#endif
#include "hwinfo/ram.h"

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
int64_t RAM::free_Bytes() const { return _free_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t RAM::available_Bytes() const { return _available_Bytes; }

}  // namespace hwinfo