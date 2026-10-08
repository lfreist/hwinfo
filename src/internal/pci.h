// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Lookup of vendor and device names in a pci.ids database (https://pci-ids.ucw.cz). Not part of the public API.

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal {

struct PciNames {
  std::optional<std::string> vendor;
  std::optional<std::string> device;
};

inline PciNames lookup_pci(std::string_view database, std::uint16_t vendor_id, std::uint16_t device_id) {
  PciNames names;
  bool in_vendor = false;
  for (const auto line : lines(database)) {
    if (line.empty() || line.starts_with('#') || line.starts_with("\t\t")) {
      continue;
    }
    if (line.starts_with("C ")) {
      break;
    }
    const bool is_device = line.starts_with('\t');
    if (!is_device && in_vendor) {
      break;  // next vendor: device not listed
    }
    const auto entry = trim(line);
    const auto separator = entry.find("  ");
    if (separator == std::string_view::npos) {
      continue;
    }
    const auto id = parse<std::uint16_t>(entry.substr(0, separator), 16);
    if (!id) {
      continue;
    }
    if (!is_device && *id == vendor_id) {
      names.vendor = std::string(trim(entry.substr(separator)));
      in_vendor = true;
    } else if (is_device && in_vendor && *id == device_id) {
      names.device = std::string(trim(entry.substr(separator)));
      break;
    }
  }
  return names;
}

}  // namespace hwinfo::internal
