// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <format>
#include <string>

namespace hwinfo {

// Endianness is a compile time property: use std::endian::native.
struct Os {
  std::string name{};          // e.g. "Ubuntu", "Microsoft Windows 11 Pro", "macOS"
  std::string version{};       // e.g. "24.04.1 LTS (Noble Numbat)", "10.0.22631", "15.1"
  std::string kernel{};        // e.g. "6.8.0-45-generic", "Darwin 24.1.0"
  std::string architecture{};  // e.g. "x86_64", "aarch64", "arm64"
  unsigned bits = 0;           // 32 or 64

  friend bool operator==(const Os&, const Os&) = default;
};

[[nodiscard]] HWINFO_API result<Os> os();

inline std::string to_string(const Os& os) {
  return std::format("{} {} ({}, kernel {})", os.name, os.version, os.architecture, os.kernel);
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Os> : hwinfo::detail::to_string_formatter<hwinfo::Os> {};
