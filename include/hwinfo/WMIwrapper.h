// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>


#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#pragma once

#include <string>
#include <vector>
#include <type_traits>

#include <Windows.h>
#include <comdef.h>
#include <WbemIdl.h>
#include <ntddscsi.h>
#pragma comment(lib, "wbemuuid.lib")

#ifndef HWINFO_WMIWRAPPER_H_
#define HWINFO_WMIWRAPPER_H_

namespace hwinfo::wmi {

template<typename T>
bool queryWMI(const std::string &WMIClass,
              std::string field,
              std::vector<T> &value,
              const std::string &serverName = "ROOT\\CIMV2") {
  std::string query("SELECT " + field + " FROM " + WMIClass);

  HRESULT hres;
  hres = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
  if (FAILED(hres)) {
    return false;
  }
  hres = CoInitializeSecurity(nullptr,
                              -1,
                              nullptr,
                              nullptr,
                              RPC_C_AUTHN_LEVEL_DEFAULT,
                              RPC_C_IMP_LEVEL_IMPERSONATE,
                              nullptr,
                              EOAC_NONE,
                              nullptr);
  if (FAILED(hres)) {
    CoUninitialize();
    return false;
  }
  IWbemLocator *pLoc = nullptr;
  hres = CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID *) &pLoc);
  if (FAILED(hres)) {
    CoUninitialize();
    return false;
  }
  IWbemServices *pSvc = nullptr;
  hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &pSvc);
  if (FAILED(hres)) {
    pLoc->Release();
    CoUninitialize();
    return false;
  }
  hres = CoSetProxyBlanket(pSvc,
                           RPC_C_AUTHN_WINNT,
                           RPC_C_AUTHZ_NONE,
                           nullptr,
                           RPC_C_AUTHN_LEVEL_CALL,
                           RPC_C_IMP_LEVEL_IMPERSONATE,
                           nullptr,
                           EOAC_NONE);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return false;
  }
  IEnumWbemClassObject *pEnumerator = nullptr;
  hres = pSvc->ExecQuery(bstr_t("WQL"),
                         bstr_t(query.c_str()),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         nullptr,
                         &pEnumerator);
  if (FAILED(hres)) {
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return false;
  }
  IWbemClassObject *pclsObj = nullptr;
  ULONG uReturn = 0;
  while (pEnumerator) {

    HRESULT Res = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

    if (!uReturn) {
      break;
    }

    VARIANT vtProp;
    Res = pclsObj->Get(std::wstring(field.begin(), field.end()).c_str(), 0, &vtProp, nullptr, nullptr);

    if (std::is_same<T, long>::value || std::is_same<T, int>::value) {
      value.push_back((T) vtProp.intVal);
    } else if (std::is_same<T, bool>::value) {
      value.push_back((T) vtProp.boolVal);
    } else if (std::is_same<T, unsigned>::value) {
      value.push_back((T) vtProp.uintVal);
    } else if (std::is_same<T, unsigned short>::value) {
      value.push_back((T) vtProp.uiVal);
    } else if (std::is_same<T, long long>::value) {
      value.push_back((T) vtProp.llVal);
    } else if (std::is_same<T, unsigned long long>::value) {
      value.push_back((T) vtProp.ullVal);
    } else {
      value.push_back((T) ((bstr_t) vtProp.bstrVal).copy());
    }

    VariantClear(&vtProp);
    pclsObj->Release();
  }

  if (!value.size()) {
    value.resize(1);
  }

  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  CoUninitialize();
  return true;
}

template<typename T>
T *AdvanceBytes(T *p, SIZE_T cb) {
  return reinterpret_cast<T *>(reinterpret_cast<BYTE *>(p) + cb);
}

class EnumLogicalProcessorInformation {
 public:
  explicit EnumLogicalProcessorInformation(LOGICAL_PROCESSOR_RELATIONSHIP Relationship)
    : m_pinfoBase(nullptr), m_pinfoCurrent(nullptr), m_cbRemaining(0) {
    DWORD cb = 0;
    if (GetLogicalProcessorInformationEx(Relationship, nullptr, &cb)) return;
    if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) return;
    m_pinfoBase = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *>
    (LocalAlloc(LMEM_FIXED, cb));
    if (!m_pinfoBase) return;
    if (!GetLogicalProcessorInformationEx(Relationship, m_pinfoBase, &cb)) return;
    m_pinfoCurrent = m_pinfoBase;
    m_cbRemaining = cb;
  }
  ~EnumLogicalProcessorInformation() { LocalFree(m_pinfoBase); }
  void MoveNext() {
    if (m_pinfoCurrent) {
      m_cbRemaining -= m_pinfoCurrent->Size;
      if (m_cbRemaining) {
        m_pinfoCurrent = AdvanceBytes(m_pinfoCurrent, m_pinfoCurrent->Size);
      } else {
        m_pinfoCurrent = nullptr;
      }
    }
  }
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *Current() { return m_pinfoCurrent; }
 private:
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *m_pinfoBase;
  SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *m_pinfoCurrent;
  DWORD m_cbRemaining;
};

}  // namespace hwinfo::wmi

#else
#error "This part of the software is Windows specific"
#endif

#endif //HWINFO_WMIWRAPPER_H_
