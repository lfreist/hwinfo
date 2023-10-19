// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS
#include <Windows.h>
#include <hwinfo/ram.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <string>
#include <vector>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
RAM::RAM() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(
      L"SELECT Capacity, ConfiguredClockSpeed, Manufacturer, SerialNumber, PartNumber FROM Win32_PhysicalMemory");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return;
  }
  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
  if (!u_return) {
    return;
  }
  VARIANT vt_prop;
  HRESULT hr;
  hr = obj->Get(L"Manufacturer", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _vendor = utils::wstring_to_std_string(vt_prop.bstrVal);
  }
  hr = obj->Get(L"partNumber", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _model = utils::wstring_to_std_string(vt_prop.bstrVal);
    // TODO: One expects an actual name of the RAM but wmi does not provide such a property...
    //       The "Name"-property of WMI returns "PhysicalMemory".
    _name = _model;
  }
  hr = obj->Get(L"Capacity", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _total_Bytes = std::stoll(utils::wstring_to_std_string(vt_prop.bstrVal));
  }
  hr = obj->Get(L"ConfiguredClockSpeed", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _frequency_Hz = static_cast<int64_t>(vt_prop.ulVal) * 1000 * 1000;
  }
  hr = obj->Get(L"SerialNumber", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _serialNumber = utils::wstring_to_std_string(vt_prop.bstrVal);
  }
  VariantClear(&vt_prop);
  obj->Release();
}

// _____________________________________________________________________________________________________________________
int64_t RAM::free_Bytes() {
  auto res = utils::WMI::query<std::string>(L"Win32_OperatingSystem", L"FreePhysicalMemory");
  if (res.empty()) {
    return -1;
  }
  return std::stoll(res[0]) * 1024;
}

// _____________________________________________________________________________________________________________________
int64_t RAM::available_Bytes() {
  // TODO: Get actual available memory size...
  return free_Bytes();
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS