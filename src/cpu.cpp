// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/cpu.h"

#include <string>
#include <vector>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
int CPU::id() const { return _id; }

// _____________________________________________________________________________________________________________________
const std::string& CPU::modelName() const { return _modelName; }

// _____________________________________________________________________________________________________________________
const std::string& CPU::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
int64_t CPU::L1CacheSize_Bytes() const { return _L1CacheSize_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t CPU::L2CacheSize_Bytes() const { return _L2CacheSize_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t CPU::L3CacheSize_Bytes() const { return _L3CacheSize_Bytes; }

// _____________________________________________________________________________________________________________________
int CPU::numPhysicalCores() const { return _numPhysicalCores; }

// _____________________________________________________________________________________________________________________
int CPU::numLogicalCores() const { return _numLogicalCores; }

// _____________________________________________________________________________________________________________________
int64_t CPU::maxClockSpeed_MHz() const { return _maxClockSpeed_MHz; }

// _____________________________________________________________________________________________________________________
int64_t CPU::regularClockSpeed_MHz() const { return _regularClockSpeed_MHz; }

// _____________________________________________________________________________________________________________________
const std::vector<std::string>& CPU::flags() const { return _flags; }

}  // namespace hwinfo