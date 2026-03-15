// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>
#include <map>

using namespace std::chrono_literals;

namespace hwinfo {

#ifdef HWINFO_UNIX
struct Jiffies {
  Jiffies() {
    working = -1;
    all = -1;
  }

  Jiffies(int64_t _all, int64_t _working) {
    all = _all;
    working = _working;
  }

  int64_t working;
  int64_t all;
};
#endif

namespace monitor::cpu {

#ifndef HWINFO_WINDOWS
void init_jiffies();
static bool jiffies_initialized = false;
#endif

double utilization(std::chrono::milliseconds sleep = 200ms);
std::vector<double> core_utilization(std::chrono::milliseconds sleep = 200ms);

}  // namespace monitor::cpu

class HWINFO_API CPU {
  friend std::vector<CPU> getAllCPUs();

 public:
  struct Core {
    std::uint64_t id;
    std::array<std::uint64_t, 3> cache_bytes;
    std::uint64_t regular_frequency_hz;
    std::uint64_t max_frequency_hz;
    bool smt;
  };

 public:
  ~CPU() = default;

  HWI_NODISCARD int id() const;
  HWI_NODISCARD const std::string& modelName() const;
  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD int numPhysicalCores() const;
  HWI_NODISCARD int numLogicalCores() const;
  HWI_NODISCARD int64_t maxClockSpeed_MHz() const;
  HWI_NODISCARD int64_t regularClockSpeed_MHz() const;
  HWI_NODISCARD std::vector<int64_t> currentClockSpeed_MHz() const;
  HWI_NODISCARD const std::vector<std::string>& flags() const;
  HWI_NODISCARD const std::vector<Core>& cores() const;

 private:
  CPU() = default;

  int _id{-1};
  std::string _modelName;
  std::string _vendor;
  int _numPhysicalCores{0};
  int _numLogicalCores{0};
  int64_t _maxClockSpeed_MHz{-1};
  int64_t _regularClockSpeed_MHz{-1};
  std::vector<std::string> _flags{};
  std::vector<Core> _cores{};
};

std::vector<CPU> getAllCPUs();

}  // namespace hwinfo
