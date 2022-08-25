// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <vector>
#include <string>
#include <regex>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include "hwinfo/utils/subprocess.h"
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")
#endif

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
GPU::GPU(const std::string& vendor, const std::string &name, const std::string &driverVersion, int64_t memory_Bytes) {
  _vendor = vendor;
  _name = name;
  _driverVersion = driverVersion;
  _memory_Bytes = memory_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string &GPU::vendor() {
  if (_vendor.empty()) {
    _vendor = getVendor();
  }
  return _vendor;
}

// _____________________________________________________________________________________________________________________
std::string &GPU::name() {
  if (_name.empty()) {
    _name = getName();
  }
  return _name;
}

// _____________________________________________________________________________________________________________________
std::string &GPU::driverVersion() {
  if (_driverVersion.empty()) {
    _driverVersion = getDriverVersion();
  }
  return _driverVersion;
}

// _____________________________________________________________________________________________________________________
int64_t GPU::memory_Bytes() {
  if (_memory_Bytes == -1) {
    _memory_Bytes = getMemory_Bytes();
  }
  return _memory_Bytes;
}

// _____________________________________________________________________________________________________________________
std::string GPU::getVendor() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  // TODO: piping stderr to /dev/null seems super ugly.
  //  Why am I doing this? -> lshw prints that one should run it as sudo user to stderr...
  std::string command("lshw -c display 2> /dev/null");
  std::regex matcher("vendor:.*");
  std::string output = exec(command);
  std::smatch match;
  std::string vendor;
  if (std::regex_search(output.cbegin(), output.cend(), match, matcher)) {
    char prev = '\0';
    bool add = false;
    std::string tmp = match[0];
    for (auto& c: tmp) {
      if (c == '\n') { break; }
      if (add) {
        vendor += c;
      }
      if (prev == ':') { add = true; }
      prev = c;
    }
  }
  return vendor.empty() ? "<unknown>" : vendor;
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_VideoController", "Name", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string GPU::getName() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  // TODO: piping stderr to /dev/null seems super ugly.
  //  Why am I doing this? -> lshw prints that one should run it as sudo user to stderr...
  std::string command("lshw -c display 2> /dev/null");
  std::regex matcher("product:.*\\[.*\\]");
  std::string output = exec(command);
  std::smatch match;
  std::string name;
  if (std::regex_search(output.cbegin(), output.cend(), match, matcher)) {
    bool add = false;
    std::string tmp = match[0];
    for (auto& c: tmp) {
      if (c == ']') { break; }
      if (add) {
        name += c;
      }
      if (c == '[') { add = true; }
    }
  }
  return name;
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_VideoController", "Name", names);
  auto ret = names[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
std::string GPU::getDriverVersion() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return "<unknown>";
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<const wchar_t*> driverVersion{};
  wmi::queryWMI("WIN32_VideoController", "DriverVersion", driverVersion);
  auto ret = driverVersion[0];
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemory_Bytes() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  return -1;
#elif defined(__APPLE__)
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  std::vector<unsigned long long> memory{};
  wmi::queryWMI("WIN32_VideoController", "AdapterRam", memory);
  return static_cast<int64_t>(memory[0] * 2);
#else
  return "<unknown>";
#endif
}

}  // namespace hwinfo