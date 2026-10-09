// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// UTF-16 <-> UTF-8 conversion for Windows APIs. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <windows.h>

#include <climits>
#include <string>
#include <string_view>

namespace hwinfo::internal {

// Converts a UTF-16 string (as returned by the W-APIs, WMI, DXGI, ...) to UTF-8.
inline std::string to_utf8(std::wstring_view s) {
  if (s.empty() || s.size() > static_cast<std::size_t>(INT_MAX)) {
    return {};
  }
  const int length = static_cast<int>(s.size());
  const int size = WideCharToMultiByte(CP_UTF8, 0, s.data(), length, nullptr, 0, nullptr, nullptr);
  if (size <= 0) {
    return {};
  }
  std::string out(static_cast<std::size_t>(size), '\0');
  WideCharToMultiByte(CP_UTF8, 0, s.data(), length, out.data(), size, nullptr, nullptr);
  return out;
}

// Converts a UTF-8 string to UTF-16.
inline std::wstring to_wide(std::string_view s) {
  if (s.empty() || s.size() > static_cast<std::size_t>(INT_MAX)) {
    return {};
  }
  const int length = static_cast<int>(s.size());
  const int size = MultiByteToWideChar(CP_UTF8, 0, s.data(), length, nullptr, 0);
  if (size <= 0) {
    return {};
  }
  std::wstring out(static_cast<std::size_t>(size), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s.data(), length, out.data(), size);
  return out;
}

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
