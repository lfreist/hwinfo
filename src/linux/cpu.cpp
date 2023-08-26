// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <algorithm>
#include <fstream>
#include <map>
#include <string>
#include <vector>
#include <unistd.h>
#include <cmath>

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cstdio>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/stringutils.h"
#include "hwinfo/utils/filesystem.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
int64_t getMaxClockSpeed_MHz(const int& core_id) {
  int64_t Hz;
  filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/scaling_max_freq", Hz);
  if (Hz > -1)
  {
    return Hz / 1000;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
int64_t getRegularClockSpeed_MHz(const int& core_id) {
  int64_t Hz;
  filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/base_frequency", Hz);
  if (Hz > -1)
  {
    return Hz / 1000;
  }

  return -1;
}

int64_t getMinClockSpeed_MHz(const int& core_id) {
  int64_t Hz;
  filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) + "/cpufreq/scaling_min_freq", Hz); 
  if (Hz > -1)
  {
    return Hz / 1000;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
int64_t CPU::currentClockSpeed_MHz() const {
  int64_t Hz;
  filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(_core_id) + "/cpufreq/scaling_cur_freq", Hz);
  if (Hz > -1)
  {
    return Hz / 1000;
  }

  return -1;
 }

double CPU::currentUtility_Percentage() const {
   // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
   //       I will not support it because I only have a 1 socket target device
  static int64_t _last_sum_all_jiffies{-1}; // Works only with 1 socket
  static int64_t _last_sum_work_jiffies{-1};

  int64_t sum_all_jiffies;
  int64_t sum_work_jiffies;
  filesystem::get_jiffies(sum_all_jiffies, sum_work_jiffies, 0);

  double total_over_period = sum_all_jiffies - _last_sum_all_jiffies;
  double work_over_period = sum_work_jiffies - _last_sum_work_jiffies;

  _last_sum_all_jiffies = sum_all_jiffies;
  _last_sum_work_jiffies = sum_work_jiffies;

  const double& percentage = work_over_period / total_over_period * 100.0;
  if (percentage < 0 || percentage > 100 || std::isnan(percentage))
  {
    return -1.0;
  }
  return percentage;
}

double CPU::currentThreadUtility_Percentage(const int& thread_index) const {
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static std::vector<int64_t> _last_sum_all_jiffies_thread(0); // Works only with 1 socket
  static std::vector<int64_t> _last_sum_work_jiffies_thread(0);
  if (_last_sum_work_jiffies_thread.empty()) {
    _last_sum_work_jiffies_thread.resize(_numLogicalCores);
  }
  if (_last_sum_all_jiffies_thread.empty()) {
    _last_sum_all_jiffies_thread.resize(_numLogicalCores);
  }

  int64_t sum_all_jiffies;
  int64_t sum_work_jiffies;
  filesystem::get_jiffies(sum_all_jiffies, sum_work_jiffies, thread_index + 1); // thread_index works only with 1 socket right now

  double total_over_period = sum_all_jiffies - _last_sum_all_jiffies_thread[thread_index];
  double work_over_period = sum_work_jiffies - _last_sum_work_jiffies_thread[thread_index];

  _last_sum_all_jiffies_thread[thread_index] = sum_all_jiffies;
  _last_sum_work_jiffies_thread[thread_index] = sum_work_jiffies;

  const double& percentage = work_over_period / total_over_period * 100.0;
  if (percentage < 0 || percentage > 100 || std::isnan(percentage))
  {
    return -1.0;
  }
  return percentage;
}

std::vector<double> CPU::currentThreadsUtility_Percentage_MainThread() const {
  std::vector<double> thread_utility(CPU::_numLogicalCores);
  for(int thread_idx = 0; thread_idx < CPU::_numLogicalCores; ++thread_idx)
  {
    thread_utility[thread_idx] = currentThreadUtility_Percentage(thread_idx);
  }
  return thread_utility;
}


double CPU::currentTemperature_Celsius() const {
    if (!std::ifstream("/etc/sensors3.conf"))
    {
      std::cout << "The lm-sensors, the tool for monitoring your system's temperature, needs to be configured. Please set it up." << std::endl;
      // Configure lm-sensors if not already configured
      std::string detect_command = "sudo sensors-detect";
      std::system(detect_command.c_str());
    }

    // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
    //       I will not support it because I only have a 1 socket target device
    const int& Socked_id = 0;

    // Command to get temperature data using 'sensors' command
    std::string command = "sensors | grep 'Package id " + std::to_string(Socked_id) + "' | awk '{print $4}'";

    // Open a pipe to execute the command and capture its output
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error executing command." << std::endl;
        return -1.0; // Return a negative value to indicate an error
    }

    char buffer[128];
    std::string result = "";
    
    // Read the output of the command into 'result'
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
    }

    // Close the pipe
    pclose(pipe);

    // Convert the result (string) to a double
    double temperature = -1.0; // Default value in case of conversion failure
    std::istringstream(result) >> temperature;
    
    return temperature;
}

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
      cpu._maxClockSpeed_MHz = getMaxClockSpeed_MHz(cpu._core_id);
      cpu._minClockSpeed_MHz = getMinClockSpeed_MHz(cpu._core_id);
      cpu._regularClockSpeed_MHz = getRegularClockSpeed_MHz(cpu._core_id);
      cpu.currentUtility_Percentage();
      cpu.currentThreadsUtility_Percentage_MainThread();
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
