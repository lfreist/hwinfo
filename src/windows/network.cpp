// Copyright (c) MRsoymilk <codermrsoymilk@gmail.com>
// This software is part of HWBenchmark

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/network.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<Network> getAllNetworks() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(
      L"SELECT InterfaceIndex, IPAddress, Description, MACAddress "
      L"FROM Win32_NetworkAdapterConfiguration");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<Network> networks;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  int network_id = 0;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    Network network;
    VARIANT vt_prop;
    HRESULT hr;
    hr = obj->Get(L"InterfaceIndex", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
        network._interfaceIndex = std::to_string(vt_prop.uintVal);
    }
    hr = obj->Get(L"IPAddress", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      if (vt_prop.vt == (VT_ARRAY | VT_BSTR)) {
        LONG lbound, ubound;
        SafeArrayGetLBound(vt_prop.parray, 1, &lbound);
        SafeArrayGetUBound(vt_prop.parray, 1, &ubound);
        std::wstring ws{};
        for (LONG i = lbound; i <= ubound; ++i) {
          BSTR bstr;
          SafeArrayGetElement(vt_prop.parray, &i, &bstr);
          ws += std::wstring(bstr, SysStringLen(bstr));
          if (i < ubound) {
            ws += L"";
          }
        }
        network._ipv4 = utils::wstring_to_std_string(ws);
      }
    }
    hr = obj->Get(L"Description", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      if (vt_prop.vt == VT_BSTR) {
        network._description = utils::wstring_to_std_string(vt_prop.bstrVal);
      }
    }
    hr = obj->Get(L"MACAddress", 0, &vt_prop, nullptr, nullptr);
    if (SUCCEEDED(hr)) {
      if (vt_prop.vt == VT_BSTR) {
        network._mac = utils::wstring_to_std_string(vt_prop.bstrVal);
      }
    }
    VariantClear(&vt_prop);
    obj->Release();
    networks.push_back(std::move(network));
  }
  return networks;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS