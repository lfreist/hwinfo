// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <string>
#include <vector>

#include "hwinfo/WMIwrapper.h"
#pragma comment(lib, "wbemuuid.lib")

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string GPU::getVendor() {
  std::vector<const wchar_t*> vendor{};
  wmi::queryWMI("WIN32_VideoController", "AdapterCompatibility", vendor);
  auto ret = vendor[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
std::string GPU::getName() {
  std::vector<const wchar_t*> names{};
  wmi::queryWMI("WIN32_VideoController", "Name", names);
  auto ret = names[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
std::string GPU::getDriverVersion() {
  std::vector<const wchar_t*> driverVersion{};
  wmi::queryWMI("WIN32_VideoController", "DriverVersion", driverVersion);
  auto ret = driverVersion[0];
  if (!ret) {
    return "<unknown>";
  }
  std::wstring tmp(ret);
  return {tmp.begin(), tmp.end()};
}

// _____________________________________________________________________________________________________________________
int64_t GPU::getMemory_Bytes() {
  std::vector<unsigned long long> memory{};
  wmi::queryWMI("WIN32_VideoController", "AdapterRam", memory);
  return static_cast<int64_t>(memory[0] * 2);
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS