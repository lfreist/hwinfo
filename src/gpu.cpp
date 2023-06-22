// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <regex>
#include <string>
#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include "hwinfo/utils/subprocess.h"
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")
#endif

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string& GPU::vendor() { return _vendor; }

// _____________________________________________________________________________________________________________________
std::string& GPU::name() { return _name; }

// _____________________________________________________________________________________________________________________
std::string& GPU::driverVersion() { return _driverVersion; }

// _____________________________________________________________________________________________________________________
int GPU::id() const { return _id; }

// _____________________________________________________________________________________________________________________
int64_t GPU::memory_Bytes() const { return _memory_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t GPU::min_frequency_MHz() const { return _minFrequency_MHz; }

// _____________________________________________________________________________________________________________________
int64_t GPU::current_frequency_MHz() const { return _currentFrequency_MHz; }

// _____________________________________________________________________________________________________________________
int64_t GPU::max_frequency_MHz() const { return _maxFrequency_MHz; }

}  // namespace hwinfo