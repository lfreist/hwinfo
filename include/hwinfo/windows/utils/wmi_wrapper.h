#include "hwinfo/utils/wmi_wrapper.h"

#ifdef HWINFO_WINDOWS

#include <algorithm>
#include <vector>

#include "hwinfo/cpu.h"
#include "hwinfo/disk.h"
#include "hwinfo/gpu.h"
#include "hwinfo/mainboard.h"
#include "hwinfo/ram.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {
namespace utils {
namespace WMI {

_WMI::_WMI() {
  auto res = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                  EOAC_NONE, NULL);
  res &= CoInitializeEx(0, COINIT_MULTITHREADED);
  res &= CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
  res &= locator->ConnectServer(_bstr_t("ROOT\\CIMV2"), NULL, NULL, 0, 0, 0, 0, &service);
  res &= CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
  if (!SUCCEEDED(res)) throw std::runtime_error("error initializing WMI");
}

_WMI::~_WMI() {
  if (locator) locator->Release();
  if (service) service->Release();
  CoUninitialize();
}

bool _WMI::execute_query(const std::wstring& query) {
  return SUCCEEDED(service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(query.begin(), query.end()).c_str()),
                                      WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator));
}

template <>
std::vector<long> query(const std::wstring& wmi_class, const std::wstring& field, const std::wstring& filter) {
  std::vector<long> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.intVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<int> WMI::query(const std::wstring& wmi_class, const std::wstring& field, const std::wstring& filter) {
  std::vector<int> result;
  for (const auto& v : query<long>(wmi_class, field, filter)) {
    result.push_back(static_cast<int>(v));
  }
  return result;
}

template <>
std::vector<bool> WMI::query(const std::wstring& wmi_class, const std::wstring& field, const std::wstring& filter) {
  std::vector<bool> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.boolVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<unsigned> WMI::query(const std::wstring& wmi_class, const std::wstring& field, const std::wstring& filter) {
  std::vector<unsigned> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.uintVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<unsigned short> WMI::query(const std::wstring& wmi_class, const std::wstring& field,
                                       const std::wstring& filter) {
  std::vector<unsigned short> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.uiVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<long long> WMI::query(const std::wstring& wmi_class, const std::wstring& field,
                                  const std::wstring& filter) {
  std::vector<long long> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.llVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<unsigned long long> WMI::query(const std::wstring& wmi_class, const std::wstring& field,
                                           const std::wstring& filter) {
  std::vector<unsigned long long> result;
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(vt_prop.ullVal);
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

template <>
std::vector<std::string> WMI::query(const std::wstring& wmi_class, const std::wstring& field,
                                    const std::wstring& filter) {
  _WMI wmi;
  std::wstring filter_string;
  if (!filter.empty()) {
    filter_string.append(L" WHERE " + filter);
  }
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class + filter_string);
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<std::string> result;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    VARIANT vt_prop;
    obj->Get(field.c_str(), 0, &vt_prop, NULL, NULL);
    result.push_back(wstring_to_std_string(vt_prop.bstrVal));
    VariantClear(&vt_prop);
    obj->Release();
  }
  return result;
}

}  // namespace WMI
}  // namespace utils
}  // namespace hwinfo

#endif  // WINDOWS