// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Reading of single registry values. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <windows.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "internal/strings.h"
#include "internal/windows_strings.h"

namespace hwinfo::internal::registry {

// REG_SZ / REG_EXPAND_SZ value, trimmed; std::nullopt if missing or empty.
inline std::optional<std::string> read_string(HKEY root, const std::wstring& subkey, const std::wstring& name) {
  DWORD size = 0;
  constexpr DWORD flags = RRF_RT_REG_SZ | RRF_RT_REG_EXPAND_SZ;
  if (RegGetValueW(root, subkey.c_str(), name.c_str(), flags, nullptr, nullptr, &size) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  std::wstring buffer(size / sizeof(wchar_t) + 1, L'\0');
  size = static_cast<DWORD>(buffer.size() * sizeof(wchar_t));
  if (RegGetValueW(root, subkey.c_str(), name.c_str(), flags, nullptr, buffer.data(), &size) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  buffer.resize(std::wstring_view(buffer.c_str()).size());  // RegGetValueW null-terminates
  return non_empty(to_utf8(buffer));
}

// REG_DWORD value; std::nullopt if missing.
inline std::optional<std::uint32_t> read_dword(HKEY root, const std::wstring& subkey, const std::wstring& name) {
  DWORD value = 0;
  DWORD size = sizeof(value);
  if (RegGetValueW(root, subkey.c_str(), name.c_str(), RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) {
    return std::nullopt;
  }
  return static_cast<std::uint32_t>(value);
}

}  // namespace hwinfo::internal::registry

#endif  // HWINFO_WINDOWS
