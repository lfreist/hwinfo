// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/stringutils.h"

#if defined(HWINFO_X86)
#include "hwinfo/cpuid.h"
#endif

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
  } catch (std::invalid_argument& e) {
    return -1;
  }
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
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return "<unknown>";
  }
  while (getline(stream, line)) {
    if (starts_with(line, "vendor_id")) {
      stream.close();
      return line.substr(line.find(": ") + 2, line.length());
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
  std::string line;
  std::ifstream stream("/proc/cpuinfo");
  if (!stream) {
    return "<unknown>";
  }
  while (getline(stream, line)) {
    if (starts_with(line, "model name")) {
      stream.close();
      return line.substr(line.find(": ") + 2, line.length());
    }
  }
  return "<unknown>";
  stream.close();
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
  } catch (std::invalid_argument& e) {
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
    if (starts_with(line, "cpu MHz")) {
      try {
        stream.close();
        return static_cast<int>(std::stof(line.substr(line.find(": ") + 2, line.length()))) * 1000;
      } catch (std::invalid_argument& e) {
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
    if (starts_with(line, "cache size")) {
      try {
        stream.close();
        return std::stoi(line.substr(line.find(": ") + 2, line.length() - 3)) * 1000;
      } catch (std::invalid_argument& e) {
        return -1;
      }
    }
  }
  stream.close();
  return -1;
}

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
// Helper function for linux: parses /proc/cpuinfo. socket_id == physical_id.
// _____________________________________________________________________________________________________________________
std::optional<CPU> getCPU(uint8_t socket_id) {
  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo.is_open()) {
    return {};
  }
  std::string file((std::istreambuf_iterator<char>(cpuinfo)), (std::istreambuf_iterator<char>()));
  cpuinfo.close();
  auto cpu_blocks_string = split(file, "\n\n");
  std::map<const std::string, const std::string> cpu_block;
  bool physical_id_found = false;
  for (const auto& block : cpu_blocks_string) {
    if (physical_id_found) {
      break;
    }
    auto lines = split(block, '\n');
    std::map<const std::string, const std::string> cpu_map;
    bool add = true;
    for (const auto& line : lines) {
      auto pair = split(line, "\t: ");
      if (pair.size() != 2) {
        continue;
      }
      strip(pair[0]);
      strip(pair[1]);
      if (pair[0] == "physical id" && pair[1] != std::to_string(socket_id)) {
        add = false;
        break;
      }
      if (pair[0] == "physical id" && pair[1] == std::to_string(socket_id)) {
        physical_id_found = true;
      }
      cpu_map.insert({std::move(pair[0]), std::move(pair[1])});
    }
    if (add && physical_id_found) {
      cpu_block = std::move(cpu_map);
      break;
    }
  }
  if (cpu_block.empty()) {
    return {};
  }
  CPU cpu;
  cpu._modelName = cpu_block["model name"];
  cpu._vendor = cpu_block["vendor_id"];
  cpu._cacheSize_Bytes = std::stoi(split(cpu_block["cache size"], ' ')[0]) * 1024;
  cpu._numPhysicalCores = std::stoi(cpu_block["cpu cores"]);
  cpu._numLogicalCores = std::stoi(cpu_block["siblings"]);
  cpu._maxClockSpeed_kHz = static_cast<int>(std::stod(cpu_block["cpu MHz"]) * 1000);
  cpu._regularClockSpeed_kHz = static_cast<int>(std::stod(cpu_block["cpu MHz"]) * 1000);

  InstructionSet instruction_set;
  std::vector<std::string> flags{"htt", "sse", "sse1", "sse2", "sse3", "sse4_1", "sse4_2", "avx", "avx2"};
  instruction_set._isHTT = (cpu_block["flags"].find(" htt ") != std::string::npos);
  instruction_set._isSSE = (cpu_block["flags"].find(" sse ") != std::string::npos);
  instruction_set._isSSE2 = (cpu_block["flags"].find(" sse2 ") != std::string::npos);
  instruction_set._isSSE3 = (cpu_block["flags"].find(" sse3 ") != std::string::npos);
  instruction_set._isSSE41 = (cpu_block["flags"].find(" sse4_1 ") != std::string::npos);
  instruction_set._isSSE42 = (cpu_block["flags"].find(" sse4_2 ") != std::string::npos);
  instruction_set._isAVX = (cpu_block["flags"].find(" avx ") != std::string::npos);
  instruction_set._isAVX2 = (cpu_block["flags"].find(" avx2 ") != std::string::npos);
  instruction_set._init_ = true;

  cpu._instructionSet = instruction_set;
  return cpu;
}

// ===== Socket ========================================================================================================
// _____________________________________________________________________________________________________________________
Socket::Socket(uint8_t id) : _id(id) {
  auto cpu = getCPU(_id);
  if (cpu.has_value()) {
    _cpu = cpu.value();
  }
}

// _____________________________________________________________________________________________________________________
Socket::Socket(uint8_t id, const class CPU& cpu) : _id(id) { _cpu = cpu; }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Socket> getAllSockets() {
  std::vector<Socket> sockets;
  int id = 0;
  while (true) {
    auto cpu = getCPU(id);
    if (!cpu.has_value()) {
      break;
    }
    sockets.emplace_back(id++, std::move(cpu.value()));
  }
  return sockets;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
