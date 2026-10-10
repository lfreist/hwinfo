// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Lookup of vendor and device names in a pci.ids database (https://pci-ids.ucw.cz) and PCI address formatting.
// Not part of the public API.

#pragma once

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

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

// "domain:bus:device.function", e.g. "0000:01:00.0"
inline std::string format_pci_address(std::uint32_t domain, std::uint32_t bus, std::uint32_t device,
                                      std::uint32_t function) {
  return std::format("{:04x}:{:02x}:{:02x}.{:x}", domain, bus, device, function);
}

// Canonical form of a PCI address: "domain:bus:device.function" with a 4 digit lowercase domain. Accepts the 8 digit
// domain of NVML ("00000000:01:00.0") and addresses without domain ("01:00.0").
inline std::optional<std::string> normalize_pci_address(std::string_view s) {
  const auto dot = s.rfind('.');
  if (dot == std::string_view::npos) {
    return std::nullopt;
  }
  std::uint32_t parts[3]{};  // domain, bus, device (from the right)
  std::size_t n = 0;
  for (const auto part : split(s.substr(0, dot), ':')) {
    if (n == 3) {
      return std::nullopt;
    }
    const auto value = parse<std::uint32_t>(part, 16);
    if (!value) {
      return std::nullopt;
    }
    parts[n++] = *value;
  }
  const auto function = parse<std::uint32_t>(s.substr(dot + 1), 16);
  if (n < 2 || !function) {
    return std::nullopt;
  }
  const std::uint32_t domain = n == 3 ? parts[0] : 0;
  const std::uint32_t bus = parts[n - 2];
  const std::uint32_t device = parts[n - 1];
  if (bus > 0xff || device > 0x1f || *function > 0x7) {
    return std::nullopt;
  }
  return format_pci_address(domain, bus, device, *function);
}

// The "pcidebug" property of macOS IOPCIDevices: "bus:device:function", optionally followed by e.g. "(0:0)".
inline std::optional<std::string> parse_pcidebug(std::string_view s) {
  s = s.substr(0, s.find('('));
  std::uint32_t bdf[3]{};
  std::size_t n = 0;
  for (const auto part : split(s, ':')) {
    const auto value = parse<std::uint32_t>(part);
    if (n == 3 || !value) {
      return std::nullopt;
    }
    bdf[n++] = *value;
  }
  if (n != 3 || bdf[0] > 0xff || bdf[1] > 0x1f || bdf[2] > 0x7) {
    return std::nullopt;
  }
  return format_pci_address(0, bdf[0], bdf[1], bdf[2]);
}

// Generation from a sysfs PCIe link speed like "16.0 GT/s PCIe".
inline std::optional<std::uint32_t> pcie_generation(std::string_view speed) {
  const auto value = parse<double>(speed.substr(0, speed.find(' ')));
  if (!value) {
    return std::nullopt;
  }
  constexpr std::pair<double, std::uint32_t> generations[] = {{2.5, 1}, {5, 2}, {8, 3}, {16, 4}, {32, 5}, {64, 6}};
  for (const auto& [gts, generation] : generations) {
    if (*value == gts) {
      return generation;
    }
  }
  return std::nullopt;
}

}  // namespace hwinfo::internal
