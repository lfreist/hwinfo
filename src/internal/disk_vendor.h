// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Disk vendor guessing. Not part of the public API.

#pragma once

#include <array>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal {

// Some interfaces (NVMe, ATA) report no or a generic vendor ("ATA", "NVMe"): guess it from the model, e.g.
// "Samsung SSD 870 EVO 1TB" -> "SAMSUNG", "WDC WD40EFRX-68N32N0" -> "WDC".
inline std::optional<std::string> disk_vendor_from_model(std::string_view model) {
  constexpr std::array<std::string_view, 12> vendors{"SAMSUNG", "WESTERN DIGITAL", "WDC",      "WD",
                                                     "SEAGATE", "KIOXIA",          "TOSHIBA",  "INTEL",
                                                     "CRUCIAL", "MICRON",          "KINGSTON", "SANDISK"};
  for (const auto vendor : vendors) {
    if (starts_with_ignore_case(model, vendor) || contains_ignore_case(model, std::format(" {}", vendor))) {
      return std::string(vendor);
    }
  }
  return std::nullopt;
}

}  // namespace hwinfo::internal
