// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// SMBIOS/DMI helpers shared by the Linux and Windows implementations. Not part of the public API.

#pragma once

#include <hwinfo/computer.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal {

// Directory of the DMI attributes exported by the Linux kernel, if any.
inline std::optional<std::filesystem::path> dmi_directory() {
  const std::array candidates{std::filesystem::path("/sys/devices/virtual/dmi/id"),
                              std::filesystem::path("/sys/class/dmi/id")};
  for (const auto& dmi : candidates) {
    std::error_code ec;
    if (std::filesystem::exists(dmi, ec)) {
      return dmi;
    }
  }
  return std::nullopt;
}

// std::nullopt for empty strings and placeholders that firmware vendors leave in unused SMBIOS fields.
inline std::optional<std::string> dmi_string(std::optional<std::string> value) {
  constexpr std::array placeholders{
      std::string_view("To Be Filled By O.E.M."),
      std::string_view("O.E.M."),
      std::string_view("OEM"),
      std::string_view("System Product Name"),
      std::string_view("System manufacturer"),
      std::string_view("System Version"),
      std::string_view("System Serial Number"),
      std::string_view("Default string"),
      std::string_view("Not Applicable"),
      std::string_view("Not Specified"),
      std::string_view("None"),
      std::string_view("Type1ProductConfigId"),
      std::string_view("Type1Family"),
      std::string_view("0123456789"),
  };
  if (!value) {
    return std::nullopt;
  }
  const std::string_view trimmed = trim(*value);
  if (trimmed.empty() ||
      std::ranges::any_of(placeholders, [&](std::string_view p) { return equals_ignore_case(trimmed, p); })) {
    return std::nullopt;
  }
  return std::string(trimmed);
}

// SMBIOS system enclosure (type 3) chassis type. The most significant bit is a lock flag.
constexpr ChassisType chassis_from_smbios(std::uint32_t type) noexcept {
  switch (type & 0x7f) {
    case 3:  // desktop
    case 4:  // low profile desktop
    case 5:  // pizza box
    case 6:  // mini tower
    case 7:  // tower
      return ChassisType::desktop;
    case 8:   // portable
    case 9:   // laptop
    case 10:  // notebook
    case 14:  // sub notebook
    case 31:  // convertible
      return ChassisType::laptop;
    case 11:  // hand held
    case 30:  // tablet
    case 32:  // detachable
      return ChassisType::tablet;
    case 13:
      return ChassisType::all_in_one;
    case 15:  // space-saving
    case 16:  // lunch box
    case 35:  // mini PC
    case 36:  // stick PC
      return ChassisType::mini_pc;
    case 17:  // main server chassis
    case 23:  // rack mount chassis
    case 25:  // multi-system chassis
    case 28:  // blade
    case 29:  // blade enclosure
      return ChassisType::server;
    case 0:
    case 2:
      return ChassisType::unknown;
    default:
      return ChassisType::other;
  }
}

}  // namespace hwinfo::internal
