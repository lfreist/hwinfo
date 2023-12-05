
#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS
#include <hwinfo/bios.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
BIOS::BIOS() {
  utils::WMI::_WMI wmi;
  const std::wstring query_string(L"SELECT Name, SerialNumber FROM Win32_BIOS");
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
  hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _name = utils::wstring_to_std_string(vt_prop.bstrVal);
  }
  hr = obj->Get(L"SerialNumber", 0, &vt_prop, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    _serialNumber = utils::wstring_to_std_string(vt_prop.bstrVal);
  }
  VariantClear(&vt_prop);
  obj->Release();
}

} // namespace hwinfo

#endif  // HWINFO_WINDOWS
