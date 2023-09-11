// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <math.h>
#include <pthread.h>
#include <sys/sysctl.h>

#include <algorithm>
#include <string>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/cpuid.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
int CPU::currentClockSpeed_kHz() {
  // TODO: implement
  return -1;
}

// _____________________________________________________________________________________________________________________
std::string CPU::getVendor() {
#if defined(HWINFO_X86)
  std::string vendor;
  uint32_t regs[4]{0};
  cpuid::cpuid(0, 0, regs);
  vendor += std::string((const char*)&regs[1], 4);
  vendor += std::string((const char*)&regs[3], 4);
  vendor += std::string((const char*)&regs[2], 4);
  return vendor;
#else
  // TODO: implement
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string CPU::getModelName() {
#if defined(HWINFO_X86)
  std::string model;
  uint32_t regs[4]{};
  for (unsigned i = 0x80000002; i < 0x80000005; ++i) {
    cpuid::cpuid(i, 0, regs);
    for (auto c : std::string((const char*)&regs[0], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for (auto c : std::string((const char*)&regs[1], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for (auto c : std::string((const char*)&regs[2], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for (auto c : std::string((const char*)&regs[3], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
  }
  return model;
#else
  size_t size = 1024;
  std::string model;
  model.resize(size);
  if (sysctlbyname("machdep.cpu.brand_string", model.data(), &size, NULL, 0) < 0) {
    model.resize(size);
    return model;
  }
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumPhysicalCores() {
#if defined(HWINFO_X86)
  uint32_t regs[4]{};
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char& in) { in = ::toupper(in); });
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 11) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4]{};
        cpuid::cpuid(0x0b, lvl, regs_2);
        uint32_t currLevel = (LVL_TYPE & regs_2[2]) >> 8;
        if (currLevel == 0x01) {
          int numCores = getNumLogicalCores() / static_cast<int>(LVL_CORES & regs_2[1]);
          if (numCores > 0) {
            return numCores;
          }
        }
      }
    } else {
      if (HFS >= 4) {
        uint32_t regs_3[4]{};
        cpuid::cpuid(4, 0, regs_3);
        int numCores = getNumLogicalCores() / static_cast<int>(1 + ((regs_3[0] >> 26) & 0x3f));
        if (numCores > 0) {
          return numCores;
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      uint32_t regs_4[4]{};
      cpuid::cpuid(0x80000000, 0, regs_4);
      if (regs_4[0] >= 8) {
        int numCores = 1 + (regs_4[2] & 0xff);
        if (numCores > 0) {
          return numCores;
        }
      }
    }
  }
  return -1;
#else
  int physical = 0;
  size_t physical_size = sizeof(physical);
  if (sysctlbyname("hw.physicalcpu", &physical, &physical_size, nullptr, 0) != 0) {
    return -1;
  }
  return physical;
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumLogicalCores() {
#if defined(HWINFO_X86)
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char& in) { in = ::toupper(in); });
  uint32_t regs[4]{};
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 0xb) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4]{};
        cpuid::cpuid(0x0b, lvl, regs_2);
        uint32_t currLevel = (LVL_TYPE & regs_2[2]) >> 8;
        if (currLevel == 0x02) {
          return static_cast<int>(LVL_CORES & regs_2[1]);
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      cpuid::cpuid(1, 0, regs);
      return static_cast<int>(regs[1] >> 16) & 0xff;
    }
    return 1;
  }
  return -1;
#else
  int logical = 0;
  size_t logical_size = sizeof(logical);
  if (sysctlbyname("hw.logicalcpu", &logical, &logical_size, nullptr, 0) != 0) {
    return -1;
  }
  return logical;
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getMaxClockSpeed_kHz() {
  long speed = 0;
  size_t speed_size = sizeof(speed);
  if (sysctlbyname("hw.cpufrequency", &speed, &speed_size, nullptr, 0) != 0) {
    speed = -1;
  }
  return static_cast<int>(speed);
}

// _____________________________________________________________________________________________________________________
int CPU::getRegularClockSpeed_kHz() {
  uint64_t frequency = 0;
  size_t size = sizeof(frequency);
  if (sysctlbyname("hw.cpufrequency", &frequency, &size, nullptr, 0) == 0) {
    return static_cast<int>(frequency);
  }
  return -1;
}

int CPU::getCacheSize_Bytes() { return -1; }

double CPU::currentUtility_Percentage() const { return -1.0; }

double CPU::currentThreadUtility_Percentage(const int& thread_index) const { return -1.0; }

std::vector<double> CPU::currentThreadsUtility_Percentage_MainThread() const { return std::vector<double>(); }

// double CPU::currentTemperature_Celsius() const {
//  return -1.0;
// }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
// Helper function for linux: parses /proc/cpuinfo. socket_id == physical_id.
// _____________________________________________________________________________________________________________________
std::unique_ptr<CPU> getCPU(uint8_t socket_id) { return {}; }

// ===== Socket ========================================================================================================
// _____________________________________________________________________________________________________________________
Socket::Socket(uint8_t id) : _id(id) {
  auto cpu = getCPU(_id);
  if (cpu != nullptr) {
    _cpu = *cpu;
  }
}

// _____________________________________________________________________________________________________________________
Socket::Socket(uint8_t id, const class CPU& cpu) : _id(id) { _cpu = cpu; }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Socket> getAllSockets() { return {}; }

}  // namespace hwinfo

#endif  // HWINFO_APPLE