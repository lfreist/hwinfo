// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/filesystem.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
int64_t getMaxClockSpeed_MHz(const int& core_id) {
  int64_t Hz = filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) +
                                                  "/cpufreq/scaling_max_freq");
  if (Hz > -1) {
    return Hz / 1000;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
int64_t getRegularClockSpeed_MHz(const int& core_id) {
  int64_t Hz = filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) +
                                                  "/cpufreq/base_frequency");
  if (Hz > -1) {
    return Hz / 1000;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
int64_t getMinClockSpeed_MHz(const int& core_id) {
  int64_t Hz = filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) +
                                                  "/cpufreq/scaling_min_freq");
  if (Hz > -1) {
    return Hz / 1000;
  }

  return -1;
}

// _____________________________________________________________________________________________________________________
std::vector<int64_t> CPU::currentClockSpeed_MHz() const {
  std::vector<int64_t> res;
  res.reserve(numLogicalCores());
  for (int core_id = 0; /* breaks, if i is no valid cpu id */; ++core_id) {
    int64_t frequency_Hz = filesystem::get_specs_by_file_path("/sys/devices/system/cpu/cpu" + std::to_string(core_id) +
                                                              "/cpufreq/scaling_cur_freq");
    if (frequency_Hz == -1) {
      break;
    }
    res.push_back(frequency_Hz / 1000);
  }

  return res;
}

// _____________________________________________________________________________________________________________________
double CPU::currentUtilisation() const {
  init_jiffies();
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static Jiffies last = Jiffies();

  Jiffies current = filesystem::get_jiffies(0);

  auto total_over_period = static_cast<double>(current.all - last.all);
  auto work_over_period = static_cast<double>(current.working - last.working);

  last = current;

  const double utilization = work_over_period / total_over_period;
  if (utilization < 0 || utilization > 1 || std::isnan(utilization)) {
    return -1.0;
  }
  return utilization;
}

// _____________________________________________________________________________________________________________________
double CPU::threadUtilisation(int thread_index) const {
  init_jiffies();
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static std::vector<Jiffies> last(0);
  if (last.empty()) {
    last.resize(_numLogicalCores);
  }

  Jiffies current = filesystem::get_jiffies(thread_index + 1);  // thread_index works only with 1 socket right now

  auto total_over_period = static_cast<double>(current.all - last[thread_index].all);
  auto work_over_period = static_cast<double>(current.working - last[thread_index].working);

  last[thread_index] = current;

  const double percentage = work_over_period / total_over_period;
  if (percentage < 0 || percentage > 100 || std::isnan(percentage)) {
    return -1.0;
  }
  return percentage;
}

// _____________________________________________________________________________________________________________________
std::vector<double> CPU::threadsUtilisation() const {
  std::vector<double> thread_utility(CPU::_numLogicalCores);
  for (int thread_idx = 0; thread_idx < CPU::_numLogicalCores; ++thread_idx) {
    thread_utility[thread_idx] = threadUtilisation(thread_idx);
  }
  return thread_utility;
}

// _____________________________________________________________________________________________________________________
void CPU::init_jiffies() const {
  if (!_jiffies_initialized) {
    // Sleep 1 sec just for the start cause the usage needs to have a delta value which is depending on the unix file
    // read it's just for the init, you don't need to wait if the delta is already created ...
    std::this_thread::sleep_for(std::chrono::duration<double>(1));
    _jiffies_initialized = true;
  }
}

// CPU Temp -> Works | But requires Im_sensors
// double CPU::currentTemperature_Celsius() const {
//     if (!std::ifstream("/etc/sensors3.conf"))
//     {
//       std::cout << "The lm-sensors, the tool for monitoring your system's temperature, needs to be configured. Please
//       set it up." << std::endl;
//       // Configure lm-sensors if not already configured
//       std::string detect_command = "sudo sensors-detect";
//       std::system(detect_command.c_str());
//     }

//     // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
//     //       I will not support it because I only have a 1 socket target device
//     const int Socked_id = 0;

//     // Command to get temperature data using 'sensors' command
//     std::string command = "sensors | grep 'Package id " + std::to_string(Socked_id) + "' | awk '{print $4}'";

//     // Open a pipe to execute the command and capture its output
//     FILE* pipe = popen(command.c_str(), "r");
//     if (!pipe) {
//         std::cerr << "Error executing command." << std::endl;
//         return -1.0; // Return a negative value to indicate an error
//     }

//     char buffer[128];
//     std::string result = "";

//     // Read the output of the command into 'result'
//     while (!feof(pipe)) {
//         if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
//             result += buffer;
//         }
//     }

//     // Close the pipe
//     pclose(pipe);

//     // Convert the result (string) to a double
//     double temperature = -1.0; // Default value in case of conversion failure
//     std::istringstream(result) >> temperature;

//     return temperature;
// }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<CPU> getAllCPUs() {
  std::vector<CPU> cpus;

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
      if (name == "vendor_id") {
        cpu._vendor = value;
      } else if (name == "model name") {
        cpu._modelName = value;
      } else if (name == "cache size") {
        cpu._L3CacheSize_Bytes = std::stoi(utils::split(value, " ")[0]) * 1024;
      } else if (name == "siblings") {
        cpu._numLogicalCores = std::stoi(value);
      } else if (name == "cpu cores") {
        cpu._numPhysicalCores = std::stoi(value);
      } else if (name == "flags") {
        cpu._flags = utils::split(value, " ");
      } else if (name == "physical id") {
        int tmp_phys_id = std::stoi(value);
        if (physical_id == tmp_phys_id) {
          continue;
        }
        cpu._id = tmp_phys_id;
        next_add = true;
      }
    }
    if (next_add) {
      cpu._maxClockSpeed_MHz = getMaxClockSpeed_MHz(cpu._id);
      cpu._regularClockSpeed_MHz = getRegularClockSpeed_MHz(cpu._id);
      next_add = false;
      physical_id++;
      cpus.push_back(std::move(cpu));
    }
  }
  return cpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
