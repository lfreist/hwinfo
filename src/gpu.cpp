// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/gpu.h>

#include <string>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& GPU::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& GPU::name() const { return _name; }

// _____________________________________________________________________________________________________________________
const std::string& GPU::driverVersion() const { return _driverVersion; }

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