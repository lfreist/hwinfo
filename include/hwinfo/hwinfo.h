// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/battery.h>
#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/gpu.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/os.h>
#include <hwinfo/ram.h>

#include <nlohmann/json.hpp>

namespace hwinfo {

class System {
  friend class Report;

 public:
  System() = default;

 private:
  std::vector<Socket> _sockets = getAllSockets();
  std::vector<GPU> _gpus = getAllGPUs();
  OS _os{};
  RAM _ram{};
  std::vector<Disk> _disks = getAllDisks();
  MainBoard _main_board{};
};

class Report {
 public:
  Report() = default;

  std::string json_str() const;
  nlohmann::json json() const;

 private:
  const System _system;
};

}  // namespace hwinfo