// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// The udev database (/run/udev/data). Pure functions on the file content, so they can be unit tested on any platform.
// Not part of the public API.

#pragma once

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal::udev {

// The properties ("E:KEY=VALUE" lines) of a database file.
inline std::map<std::string, std::string, std::less<>> parse_properties(std::string_view content) {
  std::map<std::string, std::string, std::less<>> properties;
  for (const auto line : lines(content)) {
    if (!line.starts_with("E:")) {
      continue;
    }
    const auto entry = line.substr(2);
    const auto equals = entry.find('=');
    if (equals == std::string_view::npos || equals == 0) {
      continue;
    }
    properties.emplace(entry.substr(0, equals), entry.substr(equals + 1));
  }
  return properties;
}

// Decodes the "\xHH" escapes of *_ENC properties (ID_MODEL_ENC, ID_VENDOR_ENC, ...), e.g. "Samsung\x20SSD" ->
// "Samsung SSD". The result is trimmed; std::nullopt if empty.
inline std::optional<std::string> decode(std::string_view encoded) {
  std::string out;
  out.reserve(encoded.size());
  for (std::size_t i = 0; i < encoded.size(); ++i) {
    if (encoded[i] == '\\' && i + 3 < encoded.size() && encoded[i + 1] == 'x') {
      if (const auto code = parse<unsigned>(encoded.substr(i + 2, 2), 16)) {
        out.push_back(static_cast<char>(*code));
        i += 3;
        continue;
      }
    }
    out.push_back(encoded[i]);
  }
  return non_empty(out);
}

}  // namespace hwinfo::internal::udev
