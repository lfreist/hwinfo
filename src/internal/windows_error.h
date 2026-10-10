// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Construction of hwinfo::error from Win32 error codes and HRESULTs. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/error.h>
#include <windows.h>

#include <string>
#include <system_error>
#include <utility>

namespace hwinfo::internal {

// Win32 error code (as returned by GetLastError() or the registry / IP helper APIs).
inline error win32_error(DWORD code, std::string context) {
  return error{std::error_code(static_cast<int>(code), std::system_category()), std::move(context)};
}

// Error of the last failed Win32 call of this thread.
inline error last_error(std::string context) { return win32_error(GetLastError(), std::move(context)); }

// COM / DXGI / WMI HRESULT. std::system_category() formats HRESULTs via FormatMessage.
inline error hresult_error(HRESULT hr, std::string context) {
  return error{std::error_code(static_cast<int>(hr), std::system_category()), std::move(context)};
}

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
