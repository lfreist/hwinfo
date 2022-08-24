// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/system.h"
#include "hwinfo/WMIwrapper.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<CPU> &System::CPUs() const {
  std::vector <const wchar_t*> processorId{};
  std::vector <const wchar_t*> manufacturer{};
  std::vector <const wchar_t*> name{};
  std::vector <int> cores{};
  std::vector <int> threads{};

  wmi::queryWMI("Win32_Processor", "ProcessorId", processorId);
  wmi::queryWMI("Win32_Processor", "Manufacturer", manufacturer);
  wmi::queryWMI("Win32_Processor", "Name", name);
  wmi::queryWMI<int>("Win32_Processor", "NumberOfCores", cores);
  wmi::queryWMI<int>("Win32_Processor", "NumberOfLogicalProcessors", threads);

  for (int i = 0; i < processorId.size(); ++i) {
    CPU cpu;
    auto ret = processorId[0];
    std::wstring tmp(ret);
    std::string pid_string(tmp.begin(), tmp.end());
  }
}

}  // namespace hwinfo