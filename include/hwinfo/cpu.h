// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <hwinfo/utils/wmi_wrapper.h>

namespace hwinfo {

#ifdef HWINFO_UNIX
struct Jiffies {
  Jiffies() {
    working = -1;
    all = -1;
  }

  Jiffies(const int64_t& _all, const int64_t& _working) {
    all = _all;
    working = _working;
  }

  int64_t working{-1};
  int64_t all{-1};
};
#endif

class CPU {
  friend std::vector<CPU> utils::WMI::get_component<CPU>();
  friend class Socket;

 public:
  ~CPU() = default;

  int id() const;
  const std::string& modelName() const;
  const std::string& vendor() const;
  int64_t L1CacheSize_Bytes() const;
  int64_t L2CacheSize_Bytes() const;
  int64_t L3CacheSize_Bytes() const;
  int numPhysicalCores() const;
  int numLogicalCores() const;
  int64_t maxClockSpeed_MHz() const;
  int64_t regularClockSpeed_MHz() const;
  int64_t currentClockSpeed_MHz() const;
  double currentUtility_Percentage() const;
  double currentThreadUtility_Percentage(int thread_index) const;
  std::vector<double> currentThreadsUtility_Percentage_MainThread() const;
  // double currentTemperature_Celsius() const;
  const std::vector<std::string>& flags() const;
  void init_jiffies() const;

 private:
  CPU() = default;

  int _id {-1};
  std::string _modelName;
  std::string _vendor;
  int _numPhysicalCores{-1};
  int _numLogicalCores{-1};
  int64_t _maxClockSpeed_MHz{-1};
  int64_t _regularClockSpeed_MHz{-1};
  int64_t _L1CacheSize_Bytes{-1};
  int64_t _L2CacheSize_Bytes{-1};
  int64_t _L3CacheSize_Bytes{-1};
  std::vector<std::string> _flags{};

  mutable bool _jiffies_initialized = false;
};

}  // namespace hwinfo
