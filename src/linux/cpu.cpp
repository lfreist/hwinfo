// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

int64_t get_freq_by_id_and_type(int proc_id, const std::string& type) {
  std::string line;
  std::ifstream stream("/sys/devices/system/cpu/cpu" + std::to_string(proc_id) + "/cpufreq/" + type);
  if (!stream) {
    return -1;
  }
  getline(stream, line);
  stream.close();
  try {
    return std::stoll(line) / 1000;
  } catch (std::invalid_argument& e) {
    return -1;
  }
}

// _____________________________________________________________________________________________________________________
int64_t CPU::currentClockSpeed_MHz() const { return get_freq_by_id_and_type(_core_id, "scaling_cur_freq"); }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Socket> getAllSockets() {
  std::vector<Socket> sockets;

  std::ifstream cpuinfo("/proc/cpuinfo");
  if (!cpuinfo.is_open()) {
    return {};
  }
  std::string file((std::istreambuf_iterator<char>(cpuinfo)), (std::istreambuf_iterator<char>()));
  cpuinfo.close();
  auto cpu_blocks_string = utils::split(file, "\n\n");
  std::map<const std::string, const std::string> cpu_block;
  int physical_id = -1;
  bool next_add = false;
  for (const auto& block : cpu_blocks_string) {
    CPU cpu;
    auto lines = utils::split(block, '\n');
    for (auto& line : lines) {
      auto line_pairs = utils::split(line, ":");
      if (line_pairs.size() < 2) {
        continue;
      }
      auto name = line_pairs[0];
      auto value = line_pairs[1];
      utils::strip(name);
      utils::strip(value);
      if (name == "processor") {
        cpu._core_id = std::stoi(value);
      } else if (name == "vendor_id") {
        cpu._vendor = value;
      } else if (name == "model name") {
        cpu._modelName = value;
      } else if (name == "cache size") {
        cpu._cacheSize_Bytes = std::stoi(utils::split(value, " ")[0]) * 1024;
      } else if (name == "physical id") {
        if (physical_id == std::stoi(value)) {
          continue;
        }
        next_add = true;
      } else if (name == "siblings") {
        cpu._numLogicalCores = std::stoi(value);
      } else if (name == "cpu cores") {
        cpu._numPhysicalCores = std::stoi(value);
      } else if (name == "flags") {
        cpu._flags = utils::split(value, " ");
      }
    }
    if (next_add) {
      cpu._maxClockSpeed_MHz = get_freq_by_id_and_type(cpu._core_id, "scaling_max_freq");
      cpu._minClockSpeed_MHz = get_freq_by_id_and_type(cpu._core_id, "scaling_min_freq");
      cpu._regularClockSpeed_MHz = get_freq_by_id_and_type(cpu._core_id, "base_frequency");
      next_add = false;
      Socket socket(cpu);
      physical_id++;
      socket._id = physical_id;
      sockets.push_back(std::move(socket));
    }
  }

  return sockets;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
