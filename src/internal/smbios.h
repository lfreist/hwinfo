// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Conversions from raw SMBIOS tables (third_party/smbios) to hwinfo types. Not part of the public API.

#pragma once

#include <hwinfo/computer.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/ram.h>
#include <hwinfo/units.h>
#include <smbios/smbios.h>

#include <algorithm>
#include <cstdint>
#include <expected>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "dmi.h"
#include "uuid.h"

namespace hwinfo::internal {

// The SMBIOS table of the running system, read on first use and kept for the lifetime of the process (per component
// library): firmware does not change it while the system runs, and a missing permission stays missing. Records decoded
// from it point into the cached table and stay valid.
inline const std::expected<smbios::Table, smbios::Error>& system_table() {
  static const auto table = smbios::read_system_table();
  return table;
}

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
      .uuid = system_uuid(system->uuid.transform([](const smbios::Uuid& uuid) { return Uuid{uuid.bytes}; })),
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
    mainboard.firmware_date = bios->release_date.and_then(parse_smbios_date);
  }
  return mainboard;
}

// "SMBIOS table describes a virtual machine": BIOS characteristics extension byte 2, bit 4 (2.4). Set by QEMU,
// Hyper-V, VMware, VirtualBox and others; physical machines leave it unset.
inline bool smbios_vm_flag(const smbios::Table& table) {
  const auto bios = first_record<smbios::BiosInfo>(table);
  return bios && bios->characteristics_ext2 && (*bios->characteristics_ext2 & 0x10) != 0;
}

// SMBIOS memory type (type 17 offset 0x12, also Win32_PhysicalMemory.SMBIOSMemoryType) as e.g. "DDR5".
inline std::optional<std::string> memory_type_name(std::uint8_t type) {
  const auto memory_type = smbios::MemoryType{type};
  if (memory_type == smbios::MemoryType::other || memory_type == smbios::MemoryType::unknown) {
    return std::nullopt;
  }
  return smbios::name(memory_type).transform([](std::string_view name) { return std::string(name); });
}

constexpr MemoryFormFactor memory_form_factor(smbios::MemoryFormFactor form_factor) noexcept {
  switch (static_cast<std::uint8_t>(form_factor)) {
    case 0x00:
    case 0x02:  // unknown
      return MemoryFormFactor::unknown;
    case 0x09:  // DIMM
    case 0x0c:  // RIMM
    case 0x0f:  // FB-DIMM
      return MemoryFormFactor::dimm;
    case 0x0d:  // SODIMM
    case 0x0e:  // SRIMM
      return MemoryFormFactor::sodimm;
    case 0x11:  // CAMM (3.7)
      return MemoryFormFactor::camm;
    case 0x05:  // chip
    case 0x0a:  // TSOP
    case 0x0b:  // row of chips
    case 0x10:  // die
      return MemoryFormFactor::soldered;
    default:
      return MemoryFormFactor::other;
  }
}

// Data rate in MT/s as hwinfo frequency; 0 is unknown.
inline std::optional<Hertz> mts_frequency(std::optional<std::uint32_t> mts) {
  return mts.and_then([](std::uint32_t value) {
    return value > 0 ? std::optional(std::uint64_t{value} * FrequencyUnit::MHz) : std::nullopt;
  });
}

// Modules, slots and maximum capacity from the physical memory arrays (type 16) and memory devices (type 17). Only
// arrays used as system memory count; devices of other arrays (e.g. flash, video memory) are skipped, as are empty
// slots (size 0). total is left empty.
inline Memory memory_info(const smbios::Table& table) {
  constexpr std::uint8_t system_memory = 0x03;  // MemoryArray::use
  Memory memory;
  std::vector<std::uint16_t> other_arrays;
  for (const smbios::RawStructure s : table.structures() | smbios::of_type(smbios::MemoryArray::type_id)) {
    const auto array = smbios::MemoryArray::decode(s);
    if (!array) {
      continue;
    }
    if (array->use != system_memory) {
      other_arrays.push_back(s.handle());
      continue;
    }
    memory.slots = memory.slots.value_or(0) + array->device_count;
    if (array->max_capacity_bytes) {
      memory.max_capacity = memory.max_capacity.value_or(Bytes{}) + Bytes{*array->max_capacity_bytes};
    }
  }
  for (const smbios::MemoryDevice& device : table.get<smbios::MemoryDevice>()) {
    if (device.size_bytes == 0 || std::ranges::contains(other_arrays, device.array_handle)) {
      continue;
    }
    memory.modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(memory.modules.size()),
        .vendor = smbios_string(device.manufacturer),
        .name = smbios_string(device.device_locator).or_else([&] { return smbios_string(device.bank_locator); }),
        .model = smbios_string(device.part_number),
        .serial_number = smbios_string(device.serial_number),
        .size = device.size_bytes.transform([](std::uint64_t bytes) { return Bytes{bytes}; }),
        .frequency =
            mts_frequency(device.configured_speed_mts).or_else([&] { return mts_frequency(device.speed_mts); }),
        .max_frequency = mts_frequency(device.speed_mts),
        .type = memory_type_name(static_cast<std::uint8_t>(device.memory_type)),
        .form_factor = memory_form_factor(device.form_factor),
        .rank = device.rank,
        .data_width = device.data_width_bits,
        .total_width = device.total_width_bits,
        .voltage_mv = device.configured_voltage_mv,
    });
  }
  return memory;
}

}  // namespace hwinfo::internal
