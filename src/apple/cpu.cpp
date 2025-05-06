// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <mach/mach.h>
#include <mach/mach_time.h>
#include <sys/sysctl.h>

#include <string>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/sysctl.h"

#if defined(HWINFO_X86)
#include "hwinfo/cpuid.h"
#endif

namespace hwinfo {

// Helper functions to reduce code duplication
namespace {

// Check if the system is running on Apple Silicon
bool isAppleSilicon() {
  auto machine = utils::getSysctlString("hw.machine");
  return machine.find("arm64") != std::string::npos;
}

// Get the number of physical CPU cores
int getPhysicalCoreCount() { return utils::getSysctlValue<int>("hw.physicalcpu", 0); }

// Calculate CPU frequency for Apple Silicon - simplified version
int64_t getCpuFrequency(bool isMax = true) {
  // Try to get CPU frequency directly
  uint64_t directFreq = utils::getSysctlValue<uint64_t>(isMax ? "hw.cpufrequency_max" : "hw.cpufrequency", 0);
  if (directFreq > 0) {
    return static_cast<int64_t>(directFreq / 1000000);
  }

  // If we can't get a direct measurement, return -1
  return -1;
}

}  // anonymous namespace

// _____________________________________________________________________________________________________________________
int64_t getMaxClockSpeed_MHz(const int& core_id) { return getCpuFrequency(true); }

// _____________________________________________________________________________________________________________________
int64_t getRegularClockSpeed_MHz(const int& core_id) { return getCpuFrequency(false); }

// _____________________________________________________________________________________________________________________
int64_t getMinClockSpeed_MHz(const int& core_id) {
  return utils::getSysctlValue<uint64_t>("hw.cpufrequency_min", 0) / 1000000;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> CPU::currentClockSpeed_MHz() const {
  std::vector<int64_t> clockSpeeds;

  processor_info_array_t cpuInfo;
  mach_msg_type_number_t numCpuInfo;
  natural_t numCPUs = 0;

  kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &numCPUs, &cpuInfo, &numCpuInfo);

  if (err == KERN_SUCCESS) {
    // Get CPU frequency
    int64_t freq_mhz = utils::getSysctlValue<uint64_t>("hw.cpufrequency", 0) / 1000000;

    // Fill the vector with the frequency for each logical CPU
    clockSpeeds.resize(_numLogicalCores, freq_mhz > 0 ? freq_mhz : -1);

    // Free the processor info when done
    vm_deallocate(mach_task_self(), (vm_address_t)cpuInfo, numCpuInfo * sizeof(natural_t));
  } else {
    // If we can't get processor info, still resize the vector to match the number of cores
    clockSpeeds.resize(_numLogicalCores, -1);
  }

  return clockSpeeds;
}

// _____________________________________________________________________________________________________________________
std::string getVendor() {
#if defined(HWINFO_X86)
  std::string vendor;
  uint32_t regs[4]{0};
  cpuid::cpuid(0, 0, regs);
  vendor += std::string((const char*)&regs[1], 4);
  vendor += std::string((const char*)&regs[3], 4);
  vendor += std::string((const char*)&regs[2], 4);
  return vendor;
#else
  // Try to get vendor from sysctl
  auto vendor = utils::getSysctlString("machdep.cpu.vendor", "<unknown>");

  // Check if this is Apple Silicon
  if (vendor == "<unknown>" && isAppleSilicon()) {
    return "Apple";
  }

  return vendor;
#endif
}

// _____________________________________________________________________________________________________________________
double CPU::currentUtilisation() const {
  host_cpu_load_info_data_t cpuinfo;
  mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

  static uint64_t lastTotalTicks = 0;
  static uint64_t lastIdleTicks = 0;

  if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS) {
    uint64_t totalTicks = 0;
    for (int i_tick = 0; i_tick < CPU_STATE_MAX; i_tick++) {
      totalTicks += cpuinfo.cpu_ticks[i_tick];
    }

    uint64_t idleTicks = cpuinfo.cpu_ticks[CPU_STATE_IDLE];

    uint64_t totalTicksSinceLastTime = totalTicks - lastTotalTicks;
    uint64_t idleTicksSinceLastTime = idleTicks - lastIdleTicks;

    lastTotalTicks = totalTicks;
    lastIdleTicks = idleTicks;

    if (totalTicksSinceLastTime > 0) {
      return 1.0 - ((double)idleTicksSinceLastTime / totalTicksSinceLastTime);
    }
  }

  return -1.0;
}

// _____________________________________________________________________________________________________________________
double CPU::threadUtilisation(int thread_index) const {
  // On macOS, getting per-thread utilization requires more complex code
  // This is a simplified implementation that returns the same value for all threads
  if (thread_index >= 0 && thread_index < _numLogicalCores) {
    processor_cpu_load_info_t cpuLoad;
    mach_msg_type_number_t processorMsgCount;
    natural_t processorCount;

    kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount,
                                            (processor_info_array_t*)&cpuLoad, &processorMsgCount);

    if (err == KERN_SUCCESS && thread_index < processorCount) {
      static std::vector<uint64_t> lastTotalTicks;
      static std::vector<uint64_t> lastIdleTicks;

      // Initialize on first call
      if (lastTotalTicks.size() != processorCount) {
        lastTotalTicks.resize(processorCount, 0);
        lastIdleTicks.resize(processorCount, 0);
      }

      uint64_t totalTicks = 0;
      for (int state = 0; state < CPU_STATE_MAX; state++) {
        totalTicks += cpuLoad[thread_index].cpu_ticks[state];
      }

      uint64_t idleTicks = cpuLoad[thread_index].cpu_ticks[CPU_STATE_IDLE];

      uint64_t totalTicksSinceLastTime = totalTicks - lastTotalTicks[thread_index];
      uint64_t idleTicksSinceLastTime = idleTicks - lastIdleTicks[thread_index];

      lastTotalTicks[thread_index] = totalTicks;
      lastIdleTicks[thread_index] = idleTicks;

      vm_deallocate(mach_task_self(), (vm_address_t)cpuLoad,
                    processorMsgCount * sizeof(processor_cpu_load_info_data_t));

      if (totalTicksSinceLastTime > 0) {
        return 1.0 - ((double)idleTicksSinceLastTime / totalTicksSinceLastTime);
      }
    }

    if (err == KERN_SUCCESS) {
      vm_deallocate(mach_task_self(), (vm_address_t)cpuLoad,
                    processorMsgCount * sizeof(processor_cpu_load_info_data_t));
    }
  }

  return -1.0;
}

// _____________________________________________________________________________________________________________________
std::vector<double> CPU::threadsUtilisation() const {
  std::vector<double> thread_utility;
  processor_cpu_load_info_t cpuLoad;
  mach_msg_type_number_t processorMsgCount;
  natural_t processorCount;

  kern_return_t err = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &processorCount,
                                          (processor_info_array_t*)&cpuLoad, &processorMsgCount);

  if (err == KERN_SUCCESS) {
    static std::vector<uint64_t> lastTotalTicks;
    static std::vector<uint64_t> lastIdleTicks;

    // Initialize on first call
    if (lastTotalTicks.size() != processorCount) {
      lastTotalTicks.resize(processorCount, 0);
      lastIdleTicks.resize(processorCount, 0);
    }

    thread_utility.resize(processorCount, -1.0);

    for (natural_t i = 0; i < processorCount; i++) {
      uint64_t totalTicks = 0;
      for (int state = 0; state < CPU_STATE_MAX; state++) {
        totalTicks += cpuLoad[i].cpu_ticks[state];
      }

      uint64_t idleTicks = cpuLoad[i].cpu_ticks[CPU_STATE_IDLE];

      uint64_t totalTicksSinceLastTime = totalTicks - lastTotalTicks[i];
      uint64_t idleTicksSinceLastTime = idleTicks - lastIdleTicks[i];

      lastTotalTicks[i] = totalTicks;
      lastIdleTicks[i] = idleTicks;

      if (totalTicksSinceLastTime > 0) {
        thread_utility[i] = 1.0 - ((double)idleTicksSinceLastTime / totalTicksSinceLastTime);
      }
    }

    vm_deallocate(mach_task_self(), (vm_address_t)cpuLoad, processorMsgCount * sizeof(processor_cpu_load_info_data_t));
  }

  return thread_utility;
}

// _____________________________________________________________________________________________________________________
std::string getModelName() {
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
  std::string name = utils::getSysctlString("machdep.cpu.brand_string", "<unknown> ");
  name.pop_back();
  return name;
#endif
}

int getNumLogicalCores();

// _____________________________________________________________________________________________________________________
int getNumPhysicalCores() {
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
  return utils::getSysctlValue<int>("hw.physicalcpu", 0);
#endif
}

// _____________________________________________________________________________________________________________________
int getNumLogicalCores() {
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
  return utils::getSysctlValue<int>("hw.logicalcpu", 0);
#endif
}

int64_t getL1CacheSize_Bytes() { return utils::getSysctlValue<int64_t>("hw.l1dcachesize", -1); }

int64_t getL2CacheSize_Bytes() { return utils::getSysctlValue<int64_t>("hw.l2cachesize", -1); }

int64_t getL3CacheSize_Bytes() { return utils::getSysctlValue<int64_t>("hw.l3cachesize", -1); }

// _____________________________________________________________________________________________________________________
std::vector<CPU> getAllCPUs() {
  std::vector<CPU> cpus;
  CPU cpu;

  cpu._vendor = getVendor();
  cpu._modelName = getModelName();
  cpu._numPhysicalCores = getNumPhysicalCores();
  cpu._numLogicalCores = getNumLogicalCores();
  cpu._maxClockSpeed_MHz = getMaxClockSpeed_MHz(0);
  cpu._regularClockSpeed_MHz = getRegularClockSpeed_MHz(0);
  cpu._L1CacheSize_Bytes = getL1CacheSize_Bytes();
  cpu._L2CacheSize_Bytes = getL2CacheSize_Bytes();
  cpu._L3CacheSize_Bytes = getL3CacheSize_Bytes();

  cpus.push_back(cpu);

  return cpus;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
