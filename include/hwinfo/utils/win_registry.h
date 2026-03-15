#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <windows.h>
#include <string>
#include <vector>
#include <type_traits>

namespace hwinfo::internal::utils {

template <typename T>
T getRegistryValue(HKEY hKeyParent, const std::wstring& subkey, const std::wstring& valueName) {
  HKEY hKey;
  if (RegOpenKeyExW(hKeyParent, subkey.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
    return T{};
  }

  T result{};
  DWORD dwType = 0;
  DWORD dwSize = 0;

  // First call to get the required size and verify type
  if (RegQueryValueExW(hKey, valueName.c_str(), NULL, &dwType, NULL, &dwSize) == ERROR_SUCCESS) {

    // Specialization for Strings
    if constexpr (std::is_same_v<T, std::string>) {
      std::wstring wbuffer(dwSize / sizeof(wchar_t), L'\0');
      if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)wbuffer.data(), &dwSize) == ERROR_SUCCESS) {
        // Trim null terminator if present
        if (!wbuffer.empty() && wbuffer.back() == L'\0') wbuffer.pop_back();
        result = std::string(wbuffer.begin(), wbuffer.end());
      }
    }
    // Specialization for Integers (DWORD is 32-bit, mapped to int/int64_t)
    else if constexpr (std::is_integral_v<T>) {
      DWORD dwResult = 0;
      if (RegQueryValueExW(hKey, valueName.c_str(), NULL, NULL, (LPBYTE)&dwResult, &dwSize) == ERROR_SUCCESS) {
        result = static_cast<T>(dwResult);
      }
    }
  }

  RegCloseKey(hKey);
  return result;
}

}

#endif