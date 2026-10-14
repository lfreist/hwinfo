// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Conversions from raw SMBIOS tables (third_party/smbios) to hwinfo types. Not part of the public API.

#pragma once

#include <hwinfo/computer.h>
#include <hwinfo/mainboard.h>
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

// The first record of type T, if any.
template <typename T>
std::optional<T> first_record(const smbios::Table& table) {
  for (const T& record : table.get<T>()) {
    return record;
  }
  return std::nullopt;
}

// Identity from the system information (type 1) and the first enclosure (type 3). std::nullopt without type 1.
// boot_time is left empty.
inline std::optional<Computer> computer_identity(const smbios::Table& table) {
  const auto system = first_record<smbios::SystemInfo>(table);
  if (!system) {
    return std::nullopt;
  }
  std::optional<std::uint8_t> chassis_code;
  if (const auto enclosure = first_record<smbios::ChassisInfo>(table);
      enclosure && enclosure->type != smbios::ChassisType{0}) {
    chassis_code = static_cast<std::uint8_t>(enclosure->type);
  }
  return Computer{
      .vendor = smbios_string(system->manufacturer),
      .model = smbios_string(system->product_name),
      .family = smbios_string(system->family),
      .version = smbios_string(system->version),
      .sku = smbios_string(system->sku_number),
      .serial_number = smbios_string(system->serial_number),
      .chassis = chassis_code ? chassis_from_smbios(*chassis_code) : ChassisType::unknown,
      .chassis_code = chassis_code,
  };
}

// Baseboard (type 2) and firmware (type 0). std::nullopt if the table has neither.
inline std::optional<Mainboard> mainboard_info(const smbios::Table& table) {
  const auto board = first_record<smbios::BaseboardInfo>(table);
  const auto bios = first_record<smbios::BiosInfo>(table);
  if (!board && !bios) {
    return std::nullopt;
  }
  Mainboard mainboard;
  if (board) {
    mainboard.vendor = smbios_string(board->manufacturer);
    mainboard.name = smbios_string(board->product_name);
    mainboard.version = smbios_string(board->version);
    mainboard.serial_number = smbios_string(board->serial_number);
  }
  if (bios) {
    mainboard.firmware_vendor = smbios_string(bios->vendor);
    mainboard.firmware_version = smbios_string(bios->version);
  }
  return mainboard;
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
