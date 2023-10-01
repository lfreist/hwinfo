// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <hwinfo/ram.h>
#include <hwinfo/utils/stringutils.h>
#include <unistd.h>

#include <fstream>
#include <string>
#include <vector>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
struct MemInfo {
  int64_t total{-1};
  int64_t free{-1};
  int64_t available{-1};
};

void get_from_sysconf(MemInfo& mi) {
  int64_t pages = sysconf(_SC_PHYS_PAGES);
  int64_t available_pages = sysconf(_SC_AVPHYS_PAGES);
  int64_t page_size = sysconf(_SC_PAGESIZE);
  if (pages > 0 && page_size > 0) {
    mi.total = pages * page_size;
  }
  if (available_pages > 0 && page_size > 0) {
    mi.available = available_pages * page_size;
  }
}

void set_value(std::string& line, int64_t* dst) {
  auto split_line = utils::split(line, ":");
  if (split_line.size() == 2) {
    auto& value = split_line[1];
    utils::strip(value);
    auto space = value.find(' ');
    if (space != std::string::npos) {
      auto a = std::string(value.begin(), value.begin() + static_cast<int64_t>(space));
      *dst = (std::stoll(a) * 1024);
      int i = 0;
    }
  }
}

MemInfo parse_meminfo() {
  MemInfo mi;
  std::ifstream f_meminfo("/proc/meminfo");
  if (!f_meminfo) {
    get_from_sysconf(mi);
  } else {
    while (mi.total == -1 || mi.available == -1 || mi.free == -1) {
      std::string line;
      if (!std::getline(f_meminfo, line)) {
        if (mi.total == -1 || mi.available == -1) {
          get_from_sysconf(mi);
        }
        return mi;
      }
      if (utils::starts_with(line, "MemTotal")) {
        set_value(line, &mi.total);
      } else if (utils::starts_with(line, "MemFree")) {
        set_value(line, &mi.free);
      } else if (utils::starts_with(line, "MemAvailable")) {
        set_value(line, &mi.available);
      }
    }
  }
  return mi;
}

// _____________________________________________________________________________________________________________________
RAM::RAM() {
  _name = "<unknown>";
  _vendor = "<unknown>";
  _serialNumber = "<unknown>";
  _model = "<unknown>";
  auto meminfo = parse_meminfo();
  _total_Bytes = meminfo.total;
  _free_Bytes = meminfo.free;
  _available_Bytes = meminfo.available;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX