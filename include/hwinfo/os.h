// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <format>
#include <string>
#include <string_view>

namespace hwinfo {

enum class OsFamily { unknown, windows, macos, linux_, bsd };

// Endianness is a compile time property: use std::endian::native.
struct Os {
  OsFamily family = OsFamily::unknown;
  std::string name{};            // e.g. "Ubuntu", "Microsoft Windows 11 Pro", "macOS"
  std::string marketing_name{};  // e.g. "Noble Numbat", "24H2", "Sequoia"; empty if the release has none
  std::string version{};         // e.g. "24.04.1 LTS (Noble Numbat)", "10.0.22631", "15.1 (24B83)"
  std::string kernel{};          // e.g. "6.8.0-45-generic", "Darwin 24.1.0"
  std::string architecture{};    // e.g. "x86_64", "aarch64", "arm64"
  unsigned bits = 0;             // 32 or 64

  friend bool operator==(const Os&, const Os&) = default;
};

[[nodiscard]] HWINFO_API result<Os> os();

constexpr std::string_view to_string(OsFamily family) noexcept {
  switch (family) {
    case OsFamily::windows:
      return "Windows";
    case OsFamily::macos:
      return "macOS";
    case OsFamily::linux_:
      return "Linux";
    case OsFamily::bsd:
      return "BSD";
    case OsFamily::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const Os& os) {
  return std::format("{} {} ({}, kernel {})", os.name, os.version, os.architecture, os.kernel);
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::OsFamily> : hwinfo::detail::to_string_formatter<hwinfo::OsFamily> {};

template <>
struct std::formatter<hwinfo::Os> : hwinfo::detail::to_string_formatter<hwinfo::Os> {};
