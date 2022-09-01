// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <string>
#include <vector>
#include <algorithm>

#include <regex>
#include <fstream>

#include "hwinfo/cpu.h"
#include "hwinfo/cpuid.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
int CPU::currentClockSpeed_kHz() {
  std::string line;
  std::ifstream stream("/sys/devices/system/cpu/cpu" + std::to_string(0) + "/cpufreq/scaling_cur_freq");
  if (!stream) {
    return -1;
  }
  getline(stream, line);
  stream.close();
  try {
    return std::stoi(line);
  } catch (std::invalid_argument &e) {
    return -1;
  }
}

// _____________________________________________________________________________________________________________________
std::string CPU::getVendor() {
#if defined(HWINFO_X86)
  std::string vendor;
  uint32_t regs[4] {0};
  cpuid::cpuid(0, 0, regs);
  vendor += std::string((const char *) &regs[1], 4);
  vendor += std::string((const char *) &regs[3], 4);
  vendor += std::string((const char *) &regs[2], 4);
  return vendor;
#else
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return "<unknown>";
  }
  while (getline(stream, line)) {
    if (line.starts_with("vendor_id")) {
      stream.close();
      return line.substr(line.find(": ")+2, line.length());
    }
  }
  return "<unknown>";
  stream.close();
#endif
}

// _____________________________________________________________________________________________________________________
std::string CPU::getModelName() {
#if defined(HWINFO_X86)
  std::string model;
  uint32_t regs[4] {};
  for (unsigned i = 0x80000002; i < 0x80000005; ++i) {
    cpuid::cpuid(i, 0, regs);
    for(auto c : std::string((const char*)&regs[0], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[1], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[2], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&regs[3], 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
  }
  return model;
#else
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return "<unknown>";
  }
  while (getline(stream, line)) {
    if (line.starts_with("model name")) {
      stream.close();
      return line.substr(line.find(": ")+2, line.length());
    }
  }
  return "<unknown>";
  stream.close();
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumPhysicalCores() {
#if defined(HWINFO_X86)
  uint32_t regs[4] {};
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 11) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4] {};
        cpuid::cpuid(0x0b, lvl, regs_2);
        uint32_t currLevel = (LVL_TYPE & regs_2[2]) >> 8;
        if (currLevel == 0x01) {
          int numCores = getNumLogicalCores()/static_cast<int>(LVL_CORES & regs_2[1]);
          if (numCores > 0) {
            return numCores;
          }
        }
      }
    } else {
      if (HFS >= 4) {
        uint32_t regs_3[4] {};
        cpuid::cpuid(4, 0, regs_3);
        int numCores = getNumLogicalCores()/static_cast<int>(1 + ((regs_3[0] >> 26) & 0x3f));
        if (numCores > 0) {
          return numCores;
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      uint32_t regs_4[4] {};
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
#if defined(_SC_NPROCESSORS_ONLN)
  // TODO: returns number of logical cores, but I want physical cores... fix this!
  return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#else
  return -1;
#endif
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getNumLogicalCores() {
#if defined(HWINFO_X86)
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  uint32_t regs[4] {};
  cpuid::cpuid(0, 0, regs);
  uint32_t HFS = regs[0];
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 0xb) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        uint32_t regs_2[4] {};
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
#if defined(_SC_NPROCESSORS_ONLN)
  return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#else
  return -1;
#endif
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getMaxClockSpeed_kHz() {
  std::string line;
  std::ifstream stream("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
  if (!stream) {
    return -1;
  }
  getline(stream, line);
  stream.close();
  try {
    return std::stoi(line);
  } catch (std::invalid_argument &e) {
    return -1;
  }
}

// _____________________________________________________________________________________________________________________
int CPU::getRegularClockSpeed_kHz() {
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return -1;
  }
  while (getline(stream, line)) {
    if (line.starts_with("cpu MHz")) {
      try {
        stream.close();
        return static_cast<int>(std::stof(line.substr(line.find(": ")+2, line.length()))) * 1000;
      } catch (std::invalid_argument &e) {
        return -1;
      }
    }
  }
  stream.close();
  return -1;
}

int CPU::getCacheSize_Bytes() {
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return -1;
  }
  while (getline(stream, line)) {
    if (line.starts_with("cache size")) {
      try {
        stream.close();
        return std::stoi(line.substr(line.find(": ")+2, line.length()-3)) * 1000;
      } catch (std::invalid_argument &e) {
        return -1;
      }
    }
  }
  stream.close();
  return -1;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX