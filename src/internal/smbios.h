// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Conversions from raw SMBIOS tables (third_party/smbios) to hwinfo types. Not part of the public API.

#pragma once

#include <hwinfo/ram.h>
#include <hwinfo/units.h>
#include <smbios/smbios.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dmi.h"

namespace hwinfo::internal {

inline std::optional<std::string> smbios_string(std::optional<std::string_view> value) {
  return dmi_string(value.transform([](std::string_view s) { return std::string(s); }));
}

// Populated memory devices (type 17). Empty slots (size 0) are skipped; modules of unknown size are kept.
inline std::vector<MemoryModule> memory_modules(const smbios::Table& table) {
  std::vector<MemoryModule> modules;
  for (const smbios::MemoryDevice& device : table.get<smbios::MemoryDevice>()) {
    if (device.size_bytes == 0) {
      continue;
    }
    // MT/s, as reported by Win32_PhysicalMemory.ConfiguredClockSpeed on Windows
    const auto mts = device.configured_speed_mts.or_else([&] { return device.speed_mts; });
    modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(modules.size()),
        .vendor = smbios_string(device.manufacturer),
        .name = smbios_string(device.device_locator).or_else([&] { return smbios_string(device.bank_locator); }),
        .model = smbios_string(device.part_number),
        .serial_number = smbios_string(device.serial_number),
        .size = device.size_bytes.transform([](std::uint64_t bytes) { return Bytes{bytes}; }),
        .frequency = mts.and_then([](std::uint32_t value) {
          return value > 0 ? std::optional(std::uint64_t{value} * FrequencyUnit::MHz) : std::nullopt;
        }),
    });
  }
  return modules;
}

}  // namespace hwinfo::internal
