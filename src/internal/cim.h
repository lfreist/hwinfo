// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Values in the formats of the Common Information Model, as WMI reports them. Not part of the public API.

#pragma once

#include <chrono>
#include <optional>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal::cim {

// CIM_DATETIME "yyyymmddHHMMSS.mmmmmmsUUU": local time with microseconds and the offset from UTC in minutes, e.g.
// "20261012083015.500000+120". Converted to UTC, whole seconds.
inline std::optional<std::chrono::sys_seconds> parse_datetime(std::string_view s) {
  if (s.size() != 25 || s[14] != '.' || (s[21] != '+' && s[21] != '-')) {
    return std::nullopt;
  }
  const auto number = [&](std::size_t pos, std::size_t count) { return parse<int>(s.substr(pos, count)); };
  const auto year = number(0, 4);
  const auto month = number(4, 2);
  const auto day = number(6, 2);
  const auto hour = number(8, 2);
  const auto minute = number(10, 2);
  const auto second = number(12, 2);
  const auto offset = number(22, 3);
  if (!year || !month || !day || !hour || !minute || !second || !offset) {
    return std::nullopt;
  }
  const std::chrono::year_month_day date{std::chrono::year(*year), std::chrono::month(static_cast<unsigned>(*month)),
                                         std::chrono::day(static_cast<unsigned>(*day))};
  if (!date.ok() || *hour > 23 || *minute > 59 || *second > 60) {
    return std::nullopt;
  }
  const std::chrono::minutes utc_offset(s[21] == '-' ? -*offset : *offset);
  return std::chrono::sys_days(date) + std::chrono::hours(*hour) + std::chrono::minutes(*minute) +
         std::chrono::seconds(*second) - utc_offset;
}

}  // namespace hwinfo::internal::cim
