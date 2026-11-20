// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Conversions from raw SMBIOS tables (third_party/smbios) to hwinfo types. Not part of the public API.

#pragma once

#include <hwinfo/computer.h>
#include <hwinfo/error.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/ram.h>
#include <hwinfo/units.h>
#include <smbios/smbios.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "dmi.h"
#include "uuid.h"

namespace hwinfo::internal {

// The SMBIOS table of the running system, read on first use and kept for the lifetime of the process (per component
// library): firmware does not change it while the system runs, and a missing permission stays missing. Records decoded
// from it point into the cached table and stay valid.
// MSVC (release, /W4) reports C4702 "unreachable code" at the return: not for anything in the source, but for code it
// generates around the guarded static once it has inlined read_system_table().
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4702)
#endif
inline const std::expected<smbios::Table, smbios::Error>& system_table() {
  static const auto table = smbios::read_system_table();
  return table;
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

inline std::optional<std::string> smbios_string(std::optional<std::string_view> value) {
  return dmi_string(value.transform([](std::string_view s) { return std::string(s); }));
}

// The first record of type T, if any.
template <typename T>
std::optional<T> first_record(const smbios::Table& table) {
  const auto records = table.get<T>();
  return records.empty() ? std::nullopt : std::optional<T>(records.front());
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
// slots (size 0). modules stays std::nullopt if the table describes no system memory. total is left empty. A type 16 or
// 17 structure that does not decode (too short) makes the result std::nullopt if `strict`, and is skipped otherwise.
inline std::optional<Memory> decode_memory(const smbios::Table& table, bool strict) {
  constexpr std::uint8_t system_memory = 0x03;  // MemoryArray::use
  Memory memory;
  std::vector<MemoryModule> modules;
  bool described = false;  // whether the table describes system memory at all
  std::vector<std::uint16_t> other_arrays;
  for (const smbios::RawStructure s : table.structures() | smbios::of_type(smbios::MemoryArray::type_id)) {
    const auto array = smbios::MemoryArray::decode(s);
    if (!array) {
      if (strict) {
        return std::nullopt;
      }
      continue;
    }
    if (array->use != system_memory) {
      other_arrays.push_back(s.handle());
      continue;
    }
    described = true;
    memory.slots = memory.slots.value_or(0) + array->device_count;
    if (array->max_capacity_bytes) {
      memory.max_capacity = memory.max_capacity.value_or(Bytes{}) + Bytes{*array->max_capacity_bytes};
    }
  }
  for (const smbios::RawStructure s : table.structures() | smbios::of_type(smbios::MemoryDevice::type_id)) {
    const auto decoded = smbios::MemoryDevice::decode(s);
    if (!decoded) {
      if (strict) {
        return std::nullopt;
      }
      continue;
    }
    const smbios::MemoryDevice& device = *decoded;
    if (std::ranges::contains(other_arrays, device.array_handle)) {
      continue;
    }
    described = true;
    if (device.size_bytes == 0) {
      continue;  // empty slot
    }
    modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(modules.size()),
        .vendor = smbios_string(device.manufacturer),
        .name = smbios_string(device.device_locator),
        .bank = smbios_string(device.bank_locator),
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
  if (described) {
    memory.modules = std::move(modules);
  }
  return memory;
}

// Best effort: structures that do not decode are skipped.
inline Memory memory_info(const smbios::Table& table) { return *decode_memory(table, false); }

// Where a walk over the structure table stopped.
enum class TableEnd {
  end_of_table,  // after a type 127 structure
  end_of_bytes,  // the last structure ends exactly at the end of the bytes
  malformed,     // empty, or a structure is truncated: short header, length below 4, formatted area beyond the bytes
                 // or no double NUL terminating its string set
};

// Walks the structures independently of smbios::StructureView, which stops silently before a malformed structure.
inline TableEnd walk_structure_table(std::span<const std::byte> table) {
  constexpr std::size_t header_size = 4;
  constexpr std::uint8_t end_of_table = 127;
  if (table.empty()) {
    return TableEnd::malformed;
  }
  std::size_t offset = 0;
  while (offset < table.size()) {
    const auto rest = table.subspan(offset);
    if (rest.size() < header_size) {
      return TableEnd::malformed;
    }
    const auto type = std::to_integer<std::uint8_t>(rest[0]);
    const auto length = std::to_integer<std::size_t>(rest[1]);
    if (length < header_size || length > rest.size()) {
      return TableEnd::malformed;
    }
    std::size_t end = length;
    while (end + 1 < rest.size() && (rest[end] != std::byte{0} || rest[end + 1] != std::byte{0})) {
      ++end;
    }
    if (end + 1 >= rest.size()) {
      return TableEnd::malformed;
    }
    if (type == end_of_table) {
      return TableEnd::end_of_table;
    }
    offset += end + 2;
  }
  return TableEnd::end_of_bytes;
}

// A table from caller-supplied entry point and structure table bytes, e.g. the contents of
// /sys/firmware/dmi/tables/smbios_entry_point and /sys/firmware/dmi/tables/DMI. Unlike smbios::parse(), anything that
// is not a complete table is an error (errc::parse_error): an invalid entry point or checksum, a table longer than the
// entry point allows, a truncated structure, or a table that ends without type 127 before the length an SMBIOS 2.x
// entry point states. Linux trims the table after type 127, so a shorter table that ends with it is complete.
inline result<smbios::Table> table_from_bytes(std::span<const std::byte> entry_point_bytes,
                                              std::span<const std::byte> table_bytes) {
  const auto entry_point = smbios::parse_entry_point(entry_point_bytes);
  if (!entry_point || !entry_point->checksum_valid) {
    return std::unexpected(error(errc::parse_error, "SMBIOS entry point"));
  }
  const auto table_error = [] { return std::unexpected(error(errc::parse_error, "SMBIOS table")); };
  if (table_bytes.size() > entry_point->table_length) {
    return table_error();
  }
  switch (walk_structure_table(table_bytes)) {
    case TableEnd::end_of_table:
      break;
    case TableEnd::end_of_bytes:
      // 3.x states only a maximum length and requires type 127; 2.x states the exact length.
      if (entry_point->version.major >= 3 || table_bytes.size() != entry_point->table_length) {
        return table_error();
      }
      break;
    case TableEnd::malformed:
      return table_error();
  }
  auto table = smbios::parse(smbios::RawTable{
      .bytes = {table_bytes.begin(), table_bytes.end()},
      .version = entry_point->version,
      .entry_point = *entry_point,
  });
  if (!table) {
    return table_error();
  }
  return *std::move(table);
}

}  // namespace hwinfo::internal
