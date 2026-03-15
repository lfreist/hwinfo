// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <unistd.h>

#include <chrono>
#include <cmath>
#include <fstream>
#include <map>
#include <string>
#include <thread>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/utils/filesystem.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

inline uint64_t readSysfsUint(const std::string& path) {
  std::ifstream f(path);
  uint64_t val = 0;
  if (f >> val) return val;
  return 0;
}

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

namespace monitor::cpu {

// _____________________________________________________________________________________________________________________
void init_jiffies() {
  if (!jiffies_initialized) {
    // Sleep 1 sec just for the start cause the usage needs to have a delta value which is depending on the unix file
    // read it's just for the init, you don't need to wait if the delta is already created ...
    std::this_thread::sleep_for(1s);
    jiffies_initialized = true;
  }
}

// _____________________________________________________________________________________________________________________
double utilization(std::chrono::milliseconds sleep) {
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
double coreUtilization(int thread_index) {
  init_jiffies();
  // TODO: Leon Freist a socket max num and a socket id inside the CPU could make it work with all sockets
  //       I will not support it because I only have a 1 socket target device
  static std::vector<Jiffies> last(0);
  if (last.empty()) {
    last.resize(std::thread::hardware_concurrency());
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
std::vector<double> core_utilization() {
  std::vector<double> thread_utility(std::thread::hardware_concurrency());
  for (int thread_idx = 0; thread_idx < thread_utility.size(); ++thread_idx) {
    thread_utility[thread_idx] = coreUtilization(thread_idx);
  }
  return thread_utility;
}

}  // namespace monitor::cpu

// CPU Temp -> Works | But requires Im_sensors
// double CPU::currentTemperature_Celsius() const {
//     if (!std::ifstream("/etc/sensors3.conf"))
//     {
//       std::cout << "The lm-sensors, the tool for monitoring your system's temperature, needs to be configured. Please
//       set it up." << '\n';
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
//         std::cerr << "Error executing command." << '\n';
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
  std::ifstream file("/proc/cpuinfo");
  if (!file.is_open()) return {};

  std::map<int, CPU> cpuMap;
  std::string line;

  // Temporary storage for the "current" logical processor block being parsed
  std::string fallback_vendor;
  std::string fallback_model;
  auto flushBlock = [&]() {
    if (currentBlock.empty()) return;

    // Determine Physical CPU ID (Default to 0 for single-socket ARM/RISC-V)
    int physId = 0;
    if (currentBlock.count("physical id")) {
      physId = std::stoi(currentBlock["physical id"]);
    }

    if (cpuMap.find(physId) == cpuMap.end()) {
      cpuMap.insert({physId, CPU()});
    }
    CPU& cpu = cpuMap.at(physId);
    cpu._id = physId;

    if (cpu._modelName.empty()) {
      if (currentBlock.count("model name")) {
        cpu._modelName = currentBlock["model name"];
      }
      else if (currentBlock.count("cpu")) {
        cpu._modelName = currentBlock["cpu"];  // RISC-V
      }
      else if (currentBlock.count("Model")) {
        cpu._modelName = currentBlock["Model"];  // ARM
      } else {
        cpu._modelName = fallback_model;  // some ARM only have the "Hardware" entry
      }
    }

    if (cpu._vendor.empty()) {
      if (currentBlock.count("vendor_id")) {
        cpu._vendor = currentBlock["vendor_id"];
      }
      else if (currentBlock.count("CPU implementer")) {
        cpu._vendor = currentBlock["CPU implementer"];
      } else {
        cpu._vendor = fallback_vendor;
      }
    }

    // 2. Flags
    if (cpu._flags.empty()) {
      std::string fStr = currentBlock.count("flags") ? currentBlock["flags"]
                                                     : (currentBlock.count("Features")
                                                            ? currentBlock["Features"]
                                                            : (currentBlock.count("isa") ? currentBlock["isa"] : ""));
      std::stringstream ss(fStr);
      std::string flag;
      while (ss >> flag) cpu._flags.push_back(flag);
    }

    // 3. Core Logic
    int coreId = currentBlock.count("core id")
                     ? std::stoi(currentBlock["core id"])
                     : (currentBlock.count("processor") ? std::stoi(currentBlock["processor"]) : 0);

    // Check if we've already added this physical core (handle SMT)
    auto it = std::find_if(cpu._cores.begin(), cpu._cores.end(),
                           [coreId](const CPU::Core& c) { return c.id == (uint64_t)coreId; });

    if (it == cpu._cores.end()) {
      CPU::Core core;
      core.id = coreId;
      core.smt = false;  // Default

      // Check SMT via sysfs (more reliable than cpuinfo)
      std::string threadPath =
          "/sys/devices/system/cpu/cpu" + currentBlock["processor"] + "/topology/thread_siblings_list";
      std::ifstream tsf(threadPath);
      std::string siblings;
      if (tsf >> siblings && siblings.find_first_of(",-") != std::string::npos) {
        core.smt = true;
      }

      // Frequency and Cache from sysfs (Architecture Independent)
      std::string cpuPath = "/sys/devices/system/cpu/cpu" + currentBlock["processor"];
      core.max_frequency_hz = readSysfsUint(cpuPath + "/cpufreq/cpuinfo_max_freq") * 1000;
      core.regular_frequency_hz = readSysfsUint(cpuPath + "/cpufreq/base_frequency") * 1000;

      for (int i = 0; i < 3; ++i) {
        core.cache_bytes[i] =
            readSysfsUint(cpuPath + "/cache/index" + std::to_string(i + 1) + "/size") * 1024;  // Simple KB to B
      }

      cpu._cores.push_back(core);
    }

    cpu._numLogicalCores++;
    currentBlock.clear();
  };


  std::vector<std::map<std::string, std::string>> blocks;
  {
    auto emplace_block = [&blocks](std::map<std::string, std::string>&& block){
      blocks.emplace_back(std::move(block));
    };
    std::map<std::string, std::string> currentBlock;
    while (std::getline(file, line)) {
      if (line.empty()) {
        if (currentBlock.count("Processor")) {
          fallback_model = currentBlock["Processor"];
        } else if (currentBlock.count("Hardware")) {
          fallback_vendor = currentBlock["Hardware"];
        }
        blocks.emplace_back(std::move(currentBlock));
        currentBlock = {};
        continue;
      }
      auto split = utils::split(line, ':');
      if (split.size() == 2) {
        std::string key(std::move(split[0]));
        std::string val(std::move(split[1]));
        utils::strip(key);
        utils::strip(val);
        currentBlock[key] = val;
      }
    }
    if (not currentBlock.empty()) {
      if (currentBlock.count("Processor")) {
        fallback_model = currentBlock["Processor"];
      } else if (currentBlock.count("Hardware")) {
        fallback_vendor = currentBlock["Hardware"];
      }
      blocks.emplace_back(std::move(currentBlock));
    }
  }

  // Finalize CPU-level stats
  std::vector<CPU> result;
  for (auto& pair : cpuMap) {
    pair.second._numPhysicalCores = pair.second._cores.size();

    // Calculate max clock from cores
    for (const auto& c : pair.second._cores) {
      pair.second._maxClockSpeed_MHz =
          std::max(pair.second._maxClockSpeed_MHz, (int64_t)(c.max_frequency_hz / 1000000));
      pair.second._regularClockSpeed_MHz =
          std::max(pair.second._regularClockSpeed_MHz, (int64_t)(c.regular_frequency_hz / 1000000));
    }
    result.push_back(std::move(pair.second));
  }

  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
