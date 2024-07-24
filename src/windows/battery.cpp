// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#include <iostream>

#ifdef HWINFO_WINDOWS

#include <hwinfo/battery.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

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
uint32_t Battery::getEnergyFull() const { return 1; }

// _____________________________________________________________________________________________________________________
uint32_t Battery::energyNow() const { return 0; }

// _____________________________________________________________________________________________________________________
bool Battery::charging() const { return false; }

// _____________________________________________________________________________________________________________________
bool Battery::discharging() const { return false; }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Battery> getAllBatteries() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(L"SELECT DeviceID, FullChargeCapacity, Name FROM Win32_Battery");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<Battery> batteries;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int battery_id = 0;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    Battery battery;
    battery._id = battery_id++;
    VARIANT vt_prop;
    HRESULT hr;
    hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      battery._model = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    hr = obj->Get(L"FullChargeCapacity", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_I4)) {
      battery._energyFull = vt_prop.uintVal;
    }
    hr = obj->Get(L"DeviceID", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr) && (V_VT(&vt_prop) == VT_BSTR)) {
      battery._serialNumber = utils::wstring_to_std_string(vt_prop.bstrVal);
    }
    VariantClear(&vt_prop);
    obj->Release();
    batteries.push_back(std::move(battery));
  }
  return batteries;
}

}  // namespace hwinfo

#endif