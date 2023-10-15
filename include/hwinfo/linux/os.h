// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <sys/stat.h>
#include <sys/utsname.h>

#include <fstream>
#include <sstream>
#include <string>

#include "hwinfo/os.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string OS::getFullName() {
  std::string line;
  std::ifstream stream("/etc/os-release");
  if (!stream) {
    return "Linux <unknown version>";
  }
  while (getline(stream, line)) {
    if (utils::starts_with(line, "PRETTY_NAME")) {
      line = line.substr(line.find('=') + 1, line.length());
      // remove \" at begin and end of the substring result
      return {line.begin() + 1, line.end() - 1};
    }
  }
  stream.close();
  return "Linux <unknown version>";
}

// _____________________________________________________________________________________________________________________
std::string OS::getName() {
  std::string line;
  std::ifstream stream("/etc/os-release");
  if (!stream) {
    return "Linux";
  }
  while (getline(stream, line)) {
    if (utils::starts_with(line, "NAME")) {
      line = line.substr(line.find('=') + 1, line.length());
      // remove \" at begin and end of the substring result
      return {line.begin() + 1, line.end() - 1};
    }
  }
  stream.close();
  return "Linux";
}

// _____________________________________________________________________________________________________________________
std::string OS::getVersion() {
  std::string line;
  std::ifstream stream("/etc/os-release");
  if (!stream) {
    return "<unknown version>";
  }
  while (getline(stream, line)) {
    if (utils::starts_with(line, "VERSION_ID")) {
      line = line.substr(line.find('=') + 1, line.length());
      // remove \" at begin and end of the substring result
      return {line.begin() + 1, line.end() - 1};
    }
  }
  stream.close();
  return "<unknown version>";
}

// _____________________________________________________________________________________________________________________
std::string OS::getKernel() {
  static utsname info;
  if (uname(&info) == 0) {
    return info.release;
  }
  return "<unknown kernel>";
}

// _____________________________________________________________________________________________________________________
bool OS::getIs64bit() {
  struct stat buffer {};
  return (stat("/lib64/ld-linux-x86-64.so.2", &buffer) == 0);
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX