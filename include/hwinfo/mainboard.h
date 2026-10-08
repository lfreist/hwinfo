// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <format>
#include <optional>
#include <string>

namespace hwinfo {

struct Mainboard {
  std::optional<std::string> vendor{};
  std::optional<std::string> name{};
  std::optional<std::string> version{};
  std::optional<std::string> serial_number{};  // usually requires elevated privileges

  friend bool operator==(const Mainboard&, const Mainboard&) = default;
};

[[nodiscard]] HWINFO_API result<Mainboard> mainboard();

inline std::string to_string(const Mainboard& board) {
  return std::format("{} {}", board.vendor.value_or("unknown vendor"), board.name.value_or("unknown mainboard"));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Mainboard> : hwinfo::detail::to_string_formatter<hwinfo::Mainboard> {};
