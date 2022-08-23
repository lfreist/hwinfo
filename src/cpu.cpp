// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <string>
#include <vector>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <regex>
#include <fstream>
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>
#include <math.h>
#include <pthread.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <Windows.h>
#include <memory>
#endif

#include "hwinfo/cpu.h"
#include "hwinfo/cpuid.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
CPU::CPU() {
  _numCores = getNumCores();
  _numLogicalCores = getNumLogicalCores();
  _maxClockSpeedMHz = getMaxClockSpeedMHz();
  _minClockSpeedMHz = getMinClockSpeedMHz();
  _modelName = getModelName();
  _vendor = getVendor();
  _cacheSize = getCacheSize();
  _regularClockSpeedMHz = getRegularClockSpeedMHz();

  CPUID cpuid(1, 0);
  _isHTT = cpuid.EDX() & AVX_POS;
  _isSSE = cpuid.EDX() & SSE_POS;
  _isSSE2 = cpuid.EDX() & SSE2_POS;
  _isSSE3 = cpuid.ECX() & SSE3_POS;
  _isSSE41 = cpuid.ECX() & SSE41_POS;
  _isSSE42 = cpuid.ECX() & SSE42_POS;
  _isAVX = cpuid.ECX() & AVX_POS;
  cpuid = CPUID(7, 0);
  _isAVX2 = cpuid.EBX() & AVX2_POS;

}

// _____________________________________________________________________________________________________________________
CPU::~CPU() = default;

// _____________________________________________________________________________________________________________________
std::vector<int> CPU::currentClockSpeedMHz() const {
  std::vector<int> clockSpeeds(_numLogicalCores);
  for (short coreId = 0; coreId < _numLogicalCores; ++coreId) {
    clockSpeeds[coreId] = currentClockSpeedMHz(coreId);
  }
  return clockSpeeds;
}

// _____________________________________________________________________________________________________________________
int CPU::currentClockSpeedMHz(short coreId) {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string line;
  std::ifstream stream("/sys/devices/system/cpu/cpu" + std::to_string(coreId) + "/cpufreq/scaling_cur_freq");
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
#elif defined(__APPLE__)
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  return -1;
#else
#error Unsupported Platform
#endif
}

// _____________________________________________________________________________________________________________________
std::string CPU::getVendor() {
  std::string vendor;
  CPUID cpuid(0, 0);
  uint32_t HFS = cpuid.EAX();
  vendor += std::string((const char *) &cpuid.EBX(), 4);
  vendor += std::string((const char *) &cpuid.EDX(), 4);
  vendor += std::string((const char *) &cpuid.ECX(), 4);
  return vendor;
}

// _____________________________________________________________________________________________________________________
std::string CPU::getModelName() {
  std::string model;
  for (unsigned i = 0x80000002; i < 0x80000005; ++i) {
    CPUID cpuid(i, 0);
    for(auto c : std::string((const char*)&cpuid.EAX(), 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&cpuid.EBX(), 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&cpuid.ECX(), 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
    for(auto c : std::string((const char*)&cpuid.EDX(), 4)) {
      if (std::isalnum(c) || c == '(' || c == ')' || c == '@' || c == ' ' || c == '-' || c == '.') {
        model += c;
      }
    }
  }
  return model;
}

// _____________________________________________________________________________________________________________________
int CPU::getNumCores() {
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  CPUID cpuid(0, 0);
  uint32_t HFS = cpuid.EAX();
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 11) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        CPUID cpuid4(0x0b, lvl);
        uint32_t currLevel = (LVL_TYPE & cpuid4.ECX()) >> 8;
        if (currLevel == 0x01) {
          int numCores = getNumLogicalCores()/static_cast<int>(LVL_CORES & cpuid4.EBX());
          return numCores > 0 ? numCores : 1;
        }
      }
    } else {
      if (HFS >= 4) {
        int numCores = getNumLogicalCores()/static_cast<int>(1 + (CPUID(4, 0).EAX() >> 26) & 0x3f);
        return numCores > 0 ? numCores : 1;
      }
    }
    return 1;
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      if (CPUID(0x80000000, 0).EAX() >= 8) {
        int numCores = 1 + (CPUID(0x80000008, 0).ECX() & 0xff);
        return numCores > 0 ? numCores : 1;
      }
    }
    return 1;
  }
  return -1;
}

// _____________________________________________________________________________________________________________________
int CPU::getNumLogicalCores() {
  std::string vendorId = getVendor();
  std::for_each(vendorId.begin(), vendorId.end(), [](char &in) { in = ::toupper(in); } );
  CPUID cpuid(0, 0);
  uint32_t HFS = cpuid.EAX();
  if (vendorId.find("INTEL") != std::string::npos) {
    if (HFS >= 0xb) {
      for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
        CPUID cpuid4(0x0b, lvl);
        uint32_t currLevel = (LVL_TYPE & cpuid4.ECX()) >> 8;
        if (currLevel == 0x02) {
          return static_cast<int>(LVL_CORES & cpuid4.EBX());
        }
      }
    }
  } else if (vendorId.find("AMD") != std::string::npos) {
    if (HFS > 0) {
      return static_cast<int>(CPUID(1, 0).EBX() >> 16) & 0xff;
    }
    return 1;
  }
  // fallback method:
#if defined(unix) || defined(__unix) || defined(__unix__)
#ifndef _SC_NPROCESSORS_ONLN
  return -1;
#endif
  return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#elif defined(__APPLE__)
  int logical = 0;
  size_t logical_size = sizeof(logical);
  if (sysctlbyname("hw.logicalcpu", &logical, &logical_size, nullptr, 0) != 0) {
    logical = -1;
  }
  return logical;
#elif defined(_WIN32) || defined(_WIN64)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
#error Unsupported Platform
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getMaxClockSpeedMHz() {
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#elif defined(__APPLE__)
  long speed = 0;
  size_t speed_size = sizeof(speed);
  if (sysctlbyname("hw.cpufrequency", &speed, &speed_size, nullptr, 0) != 0) {
    speed = -1;
  }
  return static_cast<int>(speed / 1000 / 1000);
#elif defined(_WIN32) || defined(_WIN64)
  return -1;
#else
#error Unsupported Platform
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getRegularClockSpeedMHz() {
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#elif defined(__APPLE__)
  uint64_t frequency = 0;
  size_t size = sizeof(frequency);
  if (sysctlbyname("hw.cpufrequency", &frequency, &size, nullptr, 0) == 0) {
    return static_cast<int>(frequency / 1000 / 1000);
  }
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  HKEY hKeyProcessor;
  long lError =
    RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\SYSTEM\CentralProcessor\0)", 0, KEY_READ, &hKeyProcessor);
  if (lError != ERROR_SUCCESS) {
    return -1;
  }
  auto clearer = [](HKEY hKey) {
    RegCloseKey(hKey);
  };
  auto key = std::unique_ptr<std::remove_pointer<HKEY>::type, decltype(clearer)>(hKeyProcessor, clearer);
  DWORD dwMHz = 0;
  DWORD dwBufferSize = sizeof(DWORD);
  lError = RegQueryValueExA(key.get(), "~MHz", nullptr, nullptr, (LPBYTE) &dwMHz, &dwBufferSize);
  if (lError != ERROR_SUCCESS) {
    return -1;
  }
  return static_cast<int>(dwMHz * 1000);
#else
#error Unsupported Platform
#endif
}

// _____________________________________________________________________________________________________________________
int CPU::getMinClockSpeedMHz() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  std::string line;
  std::ifstream stream("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_min_freq");
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
#elif defined(__APPLE__)
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  return -1;
#else
#error Unsupported Platform
#endif
}

int CPU::getCacheSize() {
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#elif defined(__APPLE__)
  return -1;
#elif defined(_WIN32) || defined(_WIN64)
  return -1;
#else
#error Unsupported Platform
#endif
}

}  // namespace hwinfo