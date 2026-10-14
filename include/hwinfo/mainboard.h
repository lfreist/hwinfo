// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <chrono>
#include <format>
#include <optional>
#include <string>

namespace hwinfo {

struct Mainboard {
  std::optional<std::string> vendor{};
  std::optional<std::string> name{};
  std::optional<std::string> version{};
  std::optional<std::string> serial_number{};    // usually requires elevated privileges
  std::optional<std::string> firmware_vendor{};  // BIOS / UEFI firmware, e.g. "American Megatrends International, LLC."
  std::optional<std::string> firmware_version{};               // e.g. "N.1.10A13", "11881.1.1" (Apple)
  std::optional<std::chrono::year_month_day> firmware_date{};  // release date of the firmware; not on Apple Silicon

  friend bool operator==(const Mainboard&, const Mainboard&) = default;
};

[[nodiscard]] HWINFO_API result<Mainboard> mainboard();

inline std::string to_string(const Mainboard& board) {
  return std::format("{} {}", board.vendor.value_or("unknown vendor"), board.name.value_or("unknown mainboard"));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Mainboard> : hwinfo::detail::to_string_formatter<hwinfo::Mainboard> {};
