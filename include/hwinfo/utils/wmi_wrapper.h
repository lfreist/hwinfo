//#pragma once
//
//#include "../platform.h"
//
//#ifdef HWINFO_WINDOWS
//
//#include <WbemIdl.h>
//#include <comdef.h>
//
//#include <iostream>
//#include <string>
//#include <type_traits>
//#include <vector>
//#pragma comment(lib, "wbemuuid.lib")
//
//namespace hwinfo {
//
//namespace utils {
//namespace WMI {
//
//struct _WMI {
//  _WMI();
//  ~_WMI();
//  bool execute_query(const std::wstring& query);
//
//  IWbemLocator* locator;
//  IWbemServices* service;
//  IEnumWbemClassObject* enumerator;
//};
//
//template <typename T>
//std::vector<T> query(const std::wstring& wmi_class, const std::wstring& field, const std::wstring& filter = L"");
//
//}  // namespace WMI
//}  // namespace utils
//}  // namespace hwinfo
//
//#endif