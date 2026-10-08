// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <format>
#include <string>
#include <string_view>

namespace hwinfo::detail {

// std::formatter for types providing `to_string(const T&)` (found via ADL).
// Supports the std::string format spec.
template <typename T>
struct to_string_formatter : std::formatter<std::string_view> {
  auto format(const T& value, std::format_context& ctx) const {
    return std::formatter<std::string_view>::format(to_string(value), ctx);
  }
};

}  // namespace hwinfo::detail
