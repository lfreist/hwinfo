// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <memory>
#include <string>
#include <vector>

namespace hwinfo {

class Socket;

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
