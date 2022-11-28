// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/cpu.h"

#include <string>
#include <vector>

#include "hwinfo/cpuid.h"
#include "hwinfo/platform.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
CPU::CPU(int id) { _id = id; }

// _____________________________________________________________________________________________________________________
std::string& CPU::modelName() {
  if (_modelName.empty()) {
    _modelName = std::move(getModelName());
  }
  return _modelName;
}

// _____________________________________________________________________________________________________________________
std::string& CPU::vendor() {
  if (_vendor.empty()) {
    _vendor = std::move(getVendor());
  }
  return _vendor;
}

// _____________________________________________________________________________________________________________________
int CPU::cacheSize_Bytes() {
  if (_cacheSize_Bytes == -1) {
    _cacheSize_Bytes = getCacheSize_Bytes();
  }
  return _cacheSize_Bytes;
}

// _____________________________________________________________________________________________________________________
int CPU::numPhysicalCores() {
  if (_numPhysicalCores == -1) {
    _numPhysicalCores = getNumPhysicalCores();
  }
  return _numPhysicalCores;
}

// _____________________________________________________________________________________________________________________
int CPU::numLogicalCores() {
  if (_numLogicalCores == -1) {
    _numLogicalCores = getNumLogicalCores();
  }
  return _numLogicalCores;
}

// _____________________________________________________________________________________________________________________
int CPU::maxClockSpeed_kHz() {
  if (_maxClockSpeed_kHz == -1) {
    _maxClockSpeed_kHz = getMaxClockSpeed_kHz();
  }
  return _maxClockSpeed_kHz;
}

// _____________________________________________________________________________________________________________________
int CPU::regularClockSpeed_kHz() {
  if (_regularClockSpeed_kHz == -1) {
    _regularClockSpeed_kHz = getRegularClockSpeed_kHz();
  }
  return _regularClockSpeed_kHz;
}

// _____________________________________________________________________________________________________________________
InstructionSet& CPU::instructionSet() {
  if (!_instructionSet._init_) {
#if defined(HWINFO_X86)
    uint32_t regs[4]{};
    cpuid::cpuid(1, 0, regs);
    _instructionSet = InstructionSet{static_cast<bool>(regs[3] & AVX_POS),
                                     static_cast<bool>(regs[3] & SSE_POS),
                                     static_cast<bool>(regs[3] & SSE2_POS),
                                     static_cast<bool>(regs[2] & SSE3_POS),
                                     static_cast<bool>(regs[2] & SSE41_POS),
                                     static_cast<bool>(regs[2] & SSE42_POS),
                                     static_cast<bool>(regs[2] & AVX_POS),
                                     false,
                                     true};
    cpuid::cpuid(7, 0, regs);
    _instructionSet._isAVX2 = static_cast<bool>(regs[1] & AVX2_POS);
#else
    _instructionSet = InstructionSet();
#endif
  }
  return _instructionSet;
}

// ===== Socket ========================================================================================================
// _____________________________________________________________________________________________________________________
CPU& Socket::CPU() { return _cpu; }

}  // namespace hwinfo