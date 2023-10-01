// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <iostream>

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <iostream>

#include "hwinfo/WMIwrapper.h"
#include "hwinfo/battery.h"

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::string Battery::getVendor() const { return "<unknwon>"; }

// _____________________________________________________________________________________________________________________
std::string Battery::getModel() const { return _model; }

// _____________________________________________________________________________________________________________________
std::string Battery::getSerialNumber() const { return "<unknwon>"; }

// _____________________________________________________________________________________________________________________
std::string Battery::getTechnology() const { return "<unknwon>"; }

// _____________________________________________________________________________________________________________________
uint32_t Battery::getEnergyFull() const { return 0; }

// _____________________________________________________________________________________________________________________
uint32_t Battery::energyNow() const { return 0; }

// _____________________________________________________________________________________________________________________
bool Battery::charging() const { return false; }

// _____________________________________________________________________________________________________________________
bool Battery::discharging() const { return false; }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Battery> getAllBatteries() {
  std::vector<Battery> batteries;
  std::vector<const wchar_t*> res{};
  wmi::queryWMI("Win32_Battery", "Name", res);
  if (res.empty() || res.front() == nullptr) {
    return {};
  }
  int8_t counter = 0;
  for (const auto& v : res) {
    std::wstring tmp(v);
    batteries.emplace_back(counter++);
    batteries.back()._model = {tmp.begin(), tmp.end()};
  }
  res.clear();
  return batteries;
}

}  // namespace hwinfo

#endif