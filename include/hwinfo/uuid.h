// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <string>

namespace hwinfo {

// A UUID in canonical byte order: to_string gives the usual "8680a0a7-0400-0000-0002-000000000000" form.
struct Uuid {
  std::array<std::uint8_t, 16> bytes{};

  friend bool operator==(const Uuid&, const Uuid&) = default;
};

inline std::string to_string(const Uuid& uuid) {
  std::string out;
  for (std::size_t i = 0; i < uuid.bytes.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      out += '-';
    }
    out += std::format("{:02x}", uuid.bytes[i]);
  }
  return out;
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Uuid> : hwinfo::detail::to_string_formatter<hwinfo::Uuid> {};
