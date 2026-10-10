// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// AMD GPU knowledge and parsers for the amdgpu / KFD (ROCm kernel driver) sysfs files.
// Not part of the public API.

#pragma once

#include <cstdint>
#include <format>
#include <map>
#include <string>
#include <string_view>

#include "strings.h"

namespace hwinfo::internal::amdgpu {

inline std::string gfx_target(std::uint32_t version) {
  const std::uint32_t major = version / 10000;
  const std::uint32_t minor = version / 100 % 100;
  const std::uint32_t stepping = version % 100;
  return std::format("gfx{}{:x}{:x}", major, minor, stepping);
}

// Architecture name of a gfx IP version.
constexpr std::string_view architecture(std::uint32_t major, std::uint32_t minor, std::uint32_t stepping) noexcept {
  switch (major) {
    case 8:
      return "GCN 3";
    case 9:
      if (minor == 4) {
        return "CDNA 3";  // gfx940 - gfx942, gfx950
      }
      if (stepping == 8) {
        return "CDNA";  // gfx908 (MI100)
      }
      if (stepping == 10) {
        return "CDNA 2";  // gfx90a (MI200)
      }
      return "GCN 5";  // Vega, incl. Vega APUs
    case 10:
      return minor >= 3 ? "RDNA 2" : "RDNA";
    case 11:
      return minor >= 5 ? "RDNA 3.5" : "RDNA 3";
    case 12:
      return "RDNA 4";
    default:
      return {};
  }
}

// "key value" lines of a KFD topology "properties" file.
inline std::map<std::string, std::uint64_t, std::less<>> parse_properties(std::string_view content) {
  std::map<std::string, std::uint64_t, std::less<>> properties;
  for (const auto line : lines(content)) {
    const auto space = line.find(' ');
    if (space == std::string_view::npos) {
      continue;
    }
    if (const auto value = parse<std::uint64_t>(line.substr(space + 1))) {
      properties.emplace(std::string(line.substr(0, space)), *value);
    }
  }
  return properties;
}

// KFD "location_id" of a PCI device: bus << 8 | device << 3 | function.
constexpr std::uint64_t kfd_location_id(std::uint32_t bus, std::uint32_t device, std::uint32_t function) noexcept {
  return std::uint64_t{bus} << 8 | std::uint64_t{device} << 3 | function;
}

}  // namespace hwinfo::internal::amdgpu
