#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#include <hwinfo/cpu.h>
#include <hwinfo/disk.h>
#include <hwinfo/gpu.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/ram.h>
#include <hwinfo/utils/stringutils.h>
#include <hwinfo/utils/wmi_wrapper.h>

#include <algorithm>
#include <vector>

namespace hwinfo {
namespace utils {
namespace WMI {

void _initialize_wmi(IWbemLocator* locator) {
  CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE,
                       NULL);
  CoInitializeEx(0, COINIT_MULTITHREADED);
  CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
}

void _uninitialize_wmi(IWbemLocator* locator, IWbemServices* service) {
  if (locator) locator->Release();
  if (service) service->Release();
  CoUninitialize();
}

struct _WMI {
  _WMI() {
    auto res = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL,
                                    EOAC_NONE, NULL);
    res &= CoInitializeEx(0, COINIT_MULTITHREADED);
    res &= CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
    res &= locator->ConnectServer(_bstr_t("ROOT\\CIMV2"), NULL, NULL, 0, 0, 0, 0, &service);
    res &= CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL,
                             RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);
    if (!SUCCEEDED(res)) throw std::runtime_error("error initializing WMI");
  }

  ~_WMI() {
    if (locator) locator->Release();
    if (service) service->Release();
    CoUninitialize();
  }

  bool execute_query(const std::wstring& query) {
    return SUCCEEDED(service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(query.begin(), query.end()).c_str()),
                                        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator));
  }

  IWbemLocator* locator;
  IWbemServices* service;
  IEnumWbemClassObject* enumerator;
};

template <>
std::vector<long> query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<long> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<int> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<int> result;
  for (const auto& v : query<long>(wmi_class, field)) {
    result.push_back(static_cast<int>(v));
  }
  return result;
}

template <>
std::vector<bool> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<bool> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<unsigned> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<unsigned> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<unsigned short> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<unsigned short> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<long long> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<long long> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<unsigned long long> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  std::vector<unsigned long long> result;
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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
std::vector<std::string> WMI::query(const std::wstring& wmi_class, const std::wstring& field) {
  _WMI wmi;
  std::wstring query_string(L"SELECT " + field + L" FROM " + wmi_class);
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

// --- Query Components ---
template <>
std::vector<CPU> get_component() {
  _WMI wmi;
  const std::wstring query_string(
      L"SELECT Name, Manufacturer, NumberOfCores, NumberOfLogicalProcessors, MaxClockSpeed, L2CacheSize, L3CacheSize "
      L"FROM Win32_Processor");
  bool success = wmi.execute_query(query_string);
  if (!success) {
    return {};
  }
  std::vector<CPU> cpus;

  ULONG u_return = 0;
  IWbemClassObject* obj = nullptr;
  while (wmi.enumerator) {
    wmi.enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
    if (!u_return) {
      break;
    }
    CPU cpu;
    VARIANT vt_prop;
    obj->Get(L"Name", 0, &vt_prop, NULL, NULL);
    cpu._modelName = wstring_to_std_string(vt_prop.bstrVal);
    obj->Get(L"Manufacturer", 0, &vt_prop, NULL, NULL);
    cpu._vendor = wstring_to_std_string(vt_prop.bstrVal);
    obj->Get(L"NumberOfCores", 0, &vt_prop, NULL, NULL);
    cpu._numPhysicalCores = vt_prop.intVal;
    obj->Get(L"NumberOfLogicalProcessors", 0, &vt_prop, NULL, NULL);
    cpu._numLogicalCores = vt_prop.intVal;
    obj->Get(L"MaxClockSpeed", 0, &vt_prop, NULL, NULL);
    cpu._maxClockSpeed_MHz = vt_prop.uintVal;
    cpu._regularClockSpeed_MHz = vt_prop.uintVal;
    obj->Get(L"L2CacheSize", 0, &vt_prop, NULL, NULL);
    cpu._L2CacheSize_Bytes = vt_prop.uintVal;
    obj->Get(L"L3CacheSize", 0, &vt_prop, NULL, NULL);
    cpu._L3CacheSize_Bytes = vt_prop.uintVal;
    VariantClear(&vt_prop);
    obj->Release();
    cpus.push_back(std::move(cpu));
  }
  return cpus;
}

}  // namespace WMI
}  // namespace utils
}  // namespace hwinfo

#endif  // WINDOWS