/**
 * Copyright 2023, Leon Freist (https://github.com/lfreist)
 * Author: Leon Freist <freist.leon@gmail.com>
 *
 * This file is part of hwinfo.
 */

#include <hwinfo/hwinfo.h>

namespace hwinfo {

nlohmann::json Report::json() const {
  auto json_report = nlohmann::json::parse(R"(
    {}
  )");
  for (size_t i = 0; i < _system._sockets.size(); ++i) {
    json_report["CPU"][std::to_string(i)]["vendor"] = _system._sockets[i].CPU().vendor();
    json_report["CPU"][std::to_string(i)]["model"] = _system._sockets[i].CPU().modelName();
    json_report["CPU"][std::to_string(i)]["physical_cores"] = _system._sockets[i].CPU().numPhysicalCores();
    json_report["CPU"][std::to_string(i)]["logical_cores"] = _system._sockets[i].CPU().numLogicalCores();
    json_report["CPU"][std::to_string(i)]["cache_size_MB"] =
        _system._sockets[i].CPU().cacheSize_Bytes() / 1024;
  }
  for (size_t i = 0; i < _system._gpus.size(); ++i) {
    json_report["GPU"][std::to_string(i)]["vendor"] = _system._gpus[i].vendor();
    json_report["GPU"][std::to_string(i)]["model"] = _system._gpus[i].name();
    json_report["GPU"][std::to_string(i)]["driver_version"] = _system._gpus[i].driverVersion();
    json_report["GPU"][std::to_string(i)]["memory_MB"] = _system._gpus[i].memory_Bytes() / 1024;
  }
  for (size_t i = 0; i < _system._disks.size(); ++i) {
    json_report["Disk"][std::to_string(i)]["vendor"] = _system._disks[i].vendor();
    json_report["Disk"][std::to_string(i)]["model"] = _system._disks[i].model();
    json_report["Disk"][std::to_string(i)]["memory_MB"] = _system._disks[i].size_Bytes() / 1024;
  }
  json_report["OS"]["name"] = OS::getFullName();
  json_report["OS"]["short"] = OS::getName();
  json_report["OS"]["version"] = OS::getVersion();
  json_report["OS"]["kernel"] = OS::getKernel();
  json_report["BaseBoard"]["vendor"] = _system._main_board.vendor();
  json_report["BaseBoard"]["model"] = _system._main_board.name();
  json_report["BaseBoard"]["version"] = _system._main_board.version();

  return json_report;
}

std::string Report::json_str() const { return json().dump(); }

}  // namespace hwinfo