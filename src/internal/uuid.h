// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// UUID parsing and validation. Not part of the public API.

#pragma once

#include <hwinfo/uuid.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace hwinfo::internal {

// Parses 32 hex digits, optionally separated by dashes (e.g. "86f17d41-e257-95c0-a2aa-8c148ab96b97").
constexpr std::optional<Uuid> parse_uuid(std::string_view s) noexcept {
  Uuid uuid;
  std::size_t digits = 0;
  for (const char c : s) {
    if (c == '-') {
      continue;
    }
    int value = 0;
    if (c >= '0' && c <= '9') {
      value = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      value = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      value = c - 'A' + 10;
    } else {
      return std::nullopt;
    }
    if (digits == 32) {
      return std::nullopt;
    }
    uuid.bytes[digits / 2] = static_cast<std::uint8_t>(uuid.bytes[digits / 2] << 4 | value);
    ++digits;
  }
  return digits == 32 ? std::optional(uuid) : std::nullopt;
}

// std::nullopt for the values firmware uses for "no UUID": all zeros (not present), all ones (not set) and the
// placeholder 03000200-0400-0500-0006-000700080009 of many AMI BIOS based boards.
constexpr std::optional<Uuid> system_uuid(std::optional<Uuid> uuid) noexcept {
  if (!uuid) {
    return std::nullopt;
  }
  const auto all = [&](std::uint8_t value) {
    return std::ranges::all_of(uuid->bytes, [&](std::uint8_t b) { return b == value; });
  };
  if (all(0x00) || all(0xff) || uuid == parse_uuid("03000200-0400-0500-0006-000700080009")) {
    return std::nullopt;
  }
  return uuid;
}

}  // namespace hwinfo::internal
