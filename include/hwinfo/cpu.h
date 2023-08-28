// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace hwinfo {

class Socket;

#ifdef HWINFO_UNIX
struct Jiffies
{
  Jiffies(const int64_t& _all, const int64_t& _working) {
    all = _all;
    working = _working;
  }

  int64_t working{-1};
  int64_t all{-1};
};
#endif

class CPU {
  friend std::vector<Socket> getAllSockets();
  friend class Socket;

 public:
  ~CPU() = default;

  const std::string& modelName() const;
  const std::string& vendor() const;
  int64_t cacheSize_Bytes() const;
  int numPhysicalCores() const;
  int numLogicalCores() const;
  int64_t maxClockSpeed_MHz() const;
  int64_t regularClockSpeed_MHz() const;
  int64_t minClockSpeed_MHz() const;
  int64_t currentClockSpeed_MHz() const;
  double currentUtility_Percentage() const;
  double currentThreadUtility_Percentage(const int& thread_index) const;
  std::vector<double> currentThreadsUtility_Percentage_MainThread() const;
  // double currentTemperature_Celsius() const;
  const std::vector<std::string>& flags() const;
  int id() const;

 private:
  CPU() = default;

  std::string _modelName;
  std::string _vendor;
  int _numPhysicalCores{-1};
  int _numLogicalCores{-1};
  int64_t _maxClockSpeed_MHz{-1};
  int64_t _regularClockSpeed_MHz{-1};
  int64_t _minClockSpeed_MHz{-1};
  int64_t _cacheSize_Bytes{-1};
  std::vector<std::string> _flags{};

#ifdef HWINFO_UNIX
  bool _initialize_jiffies = false;
#endif

  int _core_id{-1};
};

class Socket {
  friend std::vector<Socket> getAllSockets();

 public:
  ~Socket() = default;
  const class CPU& CPU() const;
  int id() const;

 private:
  explicit Socket(class CPU cpu);
  int _id{-1};
  class CPU _cpu;
};

std::vector<Socket> getAllSockets();

}  // namespace hwinfo
