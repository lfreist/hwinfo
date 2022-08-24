// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <string>
#include <vector>
#include <algorithm>

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
#include <sysinfoapi.h>
#include <memory>
#include "hwinfo/WMIwrapper.h"
#endif

#include "hwinfo/cpu.h"
#include "hwinfo/cpuid.h"


namespace hwinfo {

// _____________________________________________________________________________________________________________________
CPU::CPU() {
  _numPhysicalCores = getNumPhysicalCores();
  _numLogicalCores = getNumLogicalCores();
  _maxClockSpeedMHz = getMaxClockSpeedMHz();
  _minClockSpeedMHz = getMinClockSpeedMHz();
  _modelName = getModelName();
  _vendor = getVendor();
  _cacheSizeBytes = getCacheSizeBytes();
  _regularClockSpeedMHz = getRegularClockSpeedMHz();

#if defined(HWINFO_CPUID_H_)
  uint32_t regs[4] {};
  cpuid::cpuid(1, 0, regs);
  _isHTT = static_cast<bool>(regs[3] & AVX_POS);
  _isSSE = static_cast<bool>(regs[3] & SSE_POS);
  _isSSE2 = static_cast<bool>(regs[3] & SSE2_POS);
  _isSSE3 = static_cast<bool>(regs[2] & SSE3_POS);
  _isSSE41 = static_cast<bool>(regs[2] & SSE41_POS);
  _isSSE42 = static_cast<bool>(regs[2] & SSE42_POS);
  _isAVX = static_cast<bool>(regs[2] & AVX_POS);
  cpuid::cpuid(7, 0, regs);
  _isAVX2 = static_cast<bool>(regs[1] & AVX2_POS);
#endif
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
  return -1;
}

// _____________________________________________________________________________________________________________________
std::string CPU::getVendor() {
#if !defined(HWINFO_CPUID_H_)
  std::string vendor;
  uint32_t regs[4] {0};
  cpuid::cpuid(0, 0, regs);
  vendor += std::string((const char *) &regs[1], 4);
  vendor += std::string((const char *) &regs[3], 4);
  vendor += std::string((const char *) &regs[2], 4);
  return vendor;
#endif
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#elif defined(__APPLE__)
  // TODO: implement
  return "not implemented yet...";
#elif defined(_WIN32) || defined(_WIN64)
  HKEY hKeyProcessor;
  LONG lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ, &hKeyProcessor);
  if (lError != ERROR_SUCCESS)
    return "<unknown>";

  // Smart resource cleaner pattern
  auto clearer = [](HKEY hKey) { RegCloseKey(hKey); };
  auto key = std::unique_ptr<std::remove_pointer<HKEY>::type, decltype(clearer)>(hKeyProcessor, clearer);

  CHAR pBuffer[_MAX_PATH] = { 0 };
  DWORD dwBufferSize = sizeof(pBuffer);
  lError = RegQueryValueExA(key.get(), "VendorIdentifier", nullptr, nullptr, (LPBYTE)pBuffer, &dwBufferSize);
  if (lError != ERROR_SUCCESS) {
    return "<unknown>";
  }

  return pBuffer;
#endif
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string CPU::getModelName() {
#if defined(HWINFO_CPUID_H_)
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
#endif
#if defined(unix) || defined(__unix) || defined(__unix__)
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
#elif defined(__APPLE__)
  char* model_2[1024];
  size_t size=sizeof(model_2);
  if (sysctlbyname("machdep.cpu.brand_string", model_2, &size, NULL, 0) < 0) {
      perror("sysctl");
  }
  return std::string(model);
#elif defined(_WIN32) || defined(_WIN64)
  HKEY hKeyProcessor;
  LONG lError = RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(HARDWARE\DESCRIPTION\System\CentralProcessor\0)", 0, KEY_READ, &hKeyProcessor);
  if (lError != ERROR_SUCCESS)
    return "<unknown>";

  // Smart resource cleaner pattern
  auto clearer = [](HKEY hKey) { RegCloseKey(hKey); };
  auto key = std::unique_ptr<std::remove_pointer<HKEY>::type, decltype(clearer)>(hKeyProcessor, clearer);

  CHAR pBuffer[_MAX_PATH] = { 0 };
  DWORD dwBufferSize = sizeof(pBuffer);
  lError = RegQueryValueExA(key.get(), "ProcessorNameString", nullptr, nullptr, (LPBYTE)pBuffer, &dwBufferSize);
  if (lError != ERROR_SUCCESS) {
    return "<unknown>";
  }

  return pBuffer;
#endif
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int CPU::getNumPhysicalCores() {
#if defined(HWINFO_CPUID_H_)
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
        int numCores = getNumLogicalCores()/static_cast<int>(1 + (regs_3[0] >> 26) & 0x3f);
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
#endif
#if defined(unix) || defined(__unix) || defined(__unix__)
#if defined(_SC_NPROCESSORS_ONLN)
  // TODO: returns number of logical cores... fix this!
  return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#else
  return -1;
#endif
#elif defined(__APPLE__)
  int physical = 0;
    size_t physical_size = sizeof(physical);
    if (sysctlbyname("hw.physicalcpu", &physical, &physical_size, nullptr, 0) != 0) {
      return -1;
    }
    return physical;
#elif defined(_WIN32) || defined(_WIN64)
  int count = 0;
  for (wmi::EnumLogicalProcessorInformation enumInfo(RelationProcessorCore); auto pinfo = enumInfo.Current(); enumInfo.MoveNext()) {
    int tmp = 0;
    while (tmp < pinfo->Processor.GroupCount) {
      count++;
      tmp++;
    }
  }
  return count;
#else
#error Unsupported Platform
#endif
  return -1;
}

// _____________________________________________________________________________________________________________________
int CPU::getNumLogicalCores() {
#if defined(HWINFO_CPUID_H_)
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
#endif
  // fallback method:
#if defined(unix) || defined(__unix) || defined(__unix__)
#if defined(_SC_NPROCESSORS_ONLN)
  return static_cast<int>(sysconf(_SC_NPROCESSORS_ONLN));
#else
  return -1;
#endif
#elif defined(__APPLE__)
  int logical = 0;
  size_t logical_size = sizeof(logical);
  if (sysctlbyname("hw.logicalcpu", &logical, &logical_size, nullptr, 0) != 0) {
    return -1;
  }
  return logical;
#elif defined(_WIN32) || defined(_WIN64)
  SYSTEM_INFO sysinfo;
  GetSystemInfo(&sysinfo);
  return static_cast<int>(sysinfo.dwNumberOfProcessors);
#else
#error Unsupported Platform
#endif
  return -1;
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
    return std::stoi(line) / 1000;
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
  return -1;
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
        return static_cast<int>(std::stof(line.substr(line.find(": ")+2, line.length())));
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
  return -1;
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

int CPU::getCacheSizeBytes() {
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