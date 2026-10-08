// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace hwinfo {

// Static battery information. See hwinfo/monitoring.h for charge and state.
struct Battery {
  std::uint32_t index = 0;  // position in batteries(); used by battery_status()
  std::optional<std::string> vendor{};
  std::optional<std::string> model{};
  std::optional<std::string> serial_number{};
  std::optional<std::string> technology{};
  std::optional<double> design_capacity_wh{};
  std::optional<double> full_charge_capacity_wh{};

  friend bool operator==(const Battery&, const Battery&) = default;
};

// All batteries of the system.
// An empty vector means no battery is installed.
[[nodiscard]] HWINFO_API result<std::vector<Battery>> batteries();

inline std::string to_string(const Battery& battery) {
  return std::format("{} {}", battery.vendor.value_or("unknown vendor"), battery.model.value_or("unknown battery"));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Battery> : hwinfo::detail::to_string_formatter<hwinfo::Battery> {};
