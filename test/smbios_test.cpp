// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

#include "internal/smbios.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <optional>
#include <string_view>
#include <vector>

namespace internal = hwinfo::internal;
using namespace hwinfo::literals;

namespace {

void append(std::vector<std::byte>& out, std::initializer_list<unsigned> values) {
  for (const unsigned v : values) {
    out.push_back(static_cast<std::byte>(v));
  }
}

void append(std::vector<std::byte>& out, std::string_view strings) {
  for (const char c : strings) {
    out.push_back(static_cast<std::byte>(c));
  }
}

// Structure of `type` with the formatted area `fields` (after the 4 byte header) and NUL-terminated `strings`.
void structure(std::vector<std::byte>& out, unsigned type, unsigned handle, std::initializer_list<unsigned> fields,
               std::string_view strings) {
  append(out, {type, static_cast<unsigned>(4 + fields.size()), handle & 0xff, handle >> 8});
  append(out, fields);
  append(out, strings.empty() ? std::string_view("\0", 1) : strings);
  append(out, {0});
}

// SMBIOS 2.8 memory device (type 17, 0x28 bytes). Strings: 1 device locator, 2 bank locator, 3 manufacturer,
// 4 serial number, 5 part number. `strings` holds them NUL-terminated.
struct Device {
  unsigned handle = 0;
  unsigned array = 0x1000;
  unsigned size_mib = 0;
  unsigned form_factor = 0x0d;  // SODIMM
  unsigned type = 0x22;         // DDR5
  unsigned speed = 0;
  unsigned configured_speed = 0;
  unsigned attributes = 0x01;  // rank 1
  unsigned width = 64;
  unsigned voltage_mv = 0;
  std::string_view strings;
};

void memory_device(std::vector<std::byte>& out, const Device& d) {
  std::vector<std::byte> fields;
  const auto word = [&](unsigned value) { append(fields, {value & 0xff, value >> 8}); };
  word(d.array);
  word(0xfffe);  // error information handle
  word(d.width);
  word(d.width);
  word(d.size_mib);
  append(fields, {d.form_factor, 0, 1, 2, d.type, 0x80, 0x00});
  word(d.speed);
  append(fields, {3, 4, 0, 5, d.attributes, 0, 0, 0, 0});
  word(d.configured_speed);
  word(d.voltage_mv);  // minimum
  word(d.voltage_mv);  // maximum
  word(d.voltage_mv);  // configured
  append(out, {17, static_cast<unsigned>(4 + fields.size()), d.handle & 0xff, d.handle >> 8});
  out.insert(out.end(), fields.begin(), fields.end());
  append(out, d.strings);
  append(out, {0});
}

// Physical memory array (type 16, SMBIOS 2.1 layout): location, use, error correction, maximum capacity (KiB),
// error information handle, number of devices
void memory_array(std::vector<std::byte>& out, unsigned handle, unsigned use, unsigned max_kib, unsigned devices) {
  structure(out, 16, handle,
            {0x03, use, 0x03, max_kib & 0xff, (max_kib >> 8) & 0xff, (max_kib >> 16) & 0xff, max_kib >> 24, 0xfe, 0xff,
             devices & 0xff, devices >> 8},
            "");
}

smbios::Table table() {
  std::vector<std::byte> bytes;
  using namespace std::string_view_literals;
  // BIOS (type 0): vendor, version, segment, release date, ROM size, characteristics
  structure(bytes, 0, 0x00, {1, 2, 0x00, 0xf0, 3, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
            "American Megatrends International, LLC.\0N.1.10A13\0"
            "05/14/2024\0"sv);
  // System (type 1): manufacturer, product, version, serial, UUID, wake-up type, SKU, family
  structure(bytes, 1, 0x01, {1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x06, 5, 6},
            "SchenkerTechnologiesGmbH\0SCHENKER VISION (M23)\0System Version\0NV41PZ1234\0SVS14M23\0"
            "SCHENKER VISION\0"sv);
  // Baseboard (type 2): manufacturer, product, version, serial
  structure(bytes, 2, 0x02, {1, 2, 3, 4}, "NB02\0NV4XMZ\0Not Applicable\0 \0"sv);
  // Chassis (type 3): manufacturer, type 10 (notebook) with the lock bit set, version, serial, asset tag
  structure(bytes, 3, 0x03, {1, 0x8a, 0, 0, 0}, "NB02\0"sv);
  memory_array(bytes, 0x1000, 0x03, 64 * 1024 * 1024, 4);  // system memory, 64 GiB
  memory_array(bytes, 0x1001, 0x07, 16 * 1024, 1);         // flash
  memory_device(bytes, {.handle = 0x20,
                        .size_mib = 0x2000,
                        .speed = 4800,
                        .configured_speed = 4400,
                        .voltage_mv = 1100,
                        .strings = "DIMM 0\0BANK 0\0Micron Technology\0"
                                   "33A83711\0MTC4C10163S1SC48BA1 \0"sv});
  memory_device(bytes, {.handle = 0x21, .strings = "DIMM 1\0BANK 1\0Not Specified\0Not Specified\0Not Specified\0"sv});
  memory_device(bytes, {.handle = 0x22,
                        .size_mib = 0xffff,
                        .form_factor = 0x0b,  // row of chips
                        .type = 0x02,         // unknown
                        .speed = 3200,
                        .attributes = 0,
                        .width = 0xffff,
                        .strings = "Not Specified\0BANK 2\0To Be Filled By O.E.M.\0 \0Not Specified\0"sv});
  memory_device(bytes, {.handle = 0x23, .array = 0x1001, .size_mib = 16, .type = 0x09, .strings = "BIOS\0\0\0\0\0"sv});
  append(bytes, {127, 4, 0x24, 0x00, 0, 0});
  auto parsed = smbios::parse(bytes, smbios::Version{.major = 2, .minor = 8});
  EXPECT_TRUE(parsed.has_value());
  return *std::move(parsed);
}

}  // namespace

TEST(Smbios, MemoryInfo) {
  const auto memory = internal::memory_info(table());
  EXPECT_EQ(memory.total, hwinfo::Bytes{});
  EXPECT_EQ(memory.slots, 4u);  // the flash array does not count
  EXPECT_EQ(memory.max_capacity, 64_GiB);
  ASSERT_EQ(memory.modules.size(), 2u);  // the empty slot and the flash device are skipped

  EXPECT_EQ(memory.modules[0], (hwinfo::MemoryModule{
                                   .index = 0,
                                   .vendor = "Micron Technology",
                                   .name = "DIMM 0",
                                   .model = "MTC4C10163S1SC48BA1",
                                   .serial_number = "33A83711",
                                   .size = 8_GiB,
                                   .frequency = 4400_MHz,
                                   .max_frequency = 4800_MHz,
                                   .type = "DDR5",
                                   .form_factor = hwinfo::MemoryFormFactor::sodimm,
                                   .rank = 1,
                                   .data_width = 64,
                                   .total_width = 64,
                                   .voltage_mv = 1100,
                               }));

  // Unknown size, type, rank, widths and voltage, placeholders and a missing configured speed
  EXPECT_EQ(memory.modules[1], (hwinfo::MemoryModule{
                                   .index = 1,
                                   .vendor = std::nullopt,
                                   .name = "BANK 2",
                                   .model = std::nullopt,
                                   .serial_number = std::nullopt,
                                   .size = std::nullopt,
                                   .frequency = 3200_MHz,
                                   .max_frequency = 3200_MHz,
                                   .type = std::nullopt,
                                   .form_factor = hwinfo::MemoryFormFactor::soldered,
                               }));
}

TEST(Smbios, MemoryFormFactor) {
  using hwinfo::MemoryFormFactor;
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::dimm) == MemoryFormFactor::dimm);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::fb_dimm) == MemoryFormFactor::dimm);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::sodimm) == MemoryFormFactor::sodimm);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor{0x11}) == MemoryFormFactor::camm);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::die) == MemoryFormFactor::soldered);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::unknown) == MemoryFormFactor::unknown);
  static_assert(internal::memory_form_factor(smbios::MemoryFormFactor::simm) == MemoryFormFactor::other);
  EXPECT_EQ(std::format("{}", MemoryFormFactor::sodimm), "SO-DIMM");
}

TEST(Smbios, MemoryTypeName) {
  EXPECT_EQ(internal::memory_type_name(0x1a), "DDR4");
  EXPECT_EQ(internal::memory_type_name(0x23), "LPDDR5");
  EXPECT_EQ(internal::memory_type_name(0x01), std::nullopt);  // other
  EXPECT_EQ(internal::memory_type_name(0x02), std::nullopt);  // unknown
  EXPECT_EQ(internal::memory_type_name(0xf0), std::nullopt);
}

TEST(Smbios, ComputerIdentity) {
  const auto computer = internal::computer_identity(table());
  ASSERT_TRUE(computer.has_value());
  EXPECT_EQ(computer->vendor, "SchenkerTechnologiesGmbH");
  EXPECT_EQ(computer->model, "SCHENKER VISION (M23)");
  EXPECT_EQ(computer->family, "SCHENKER VISION");
  EXPECT_EQ(computer->version, std::nullopt);  // placeholder
  EXPECT_EQ(computer->sku, "SVS14M23");
  EXPECT_EQ(computer->serial_number, "NV41PZ1234");
  EXPECT_EQ(computer->chassis, hwinfo::ChassisType::laptop);
  EXPECT_EQ(computer->chassis_code, 10);  // lock bit masked
  EXPECT_EQ(computer->boot_time, std::nullopt);
}

TEST(Smbios, MainboardInfo) {
  EXPECT_EQ(internal::mainboard_info(table()), (hwinfo::Mainboard{
                                                   .vendor = "NB02",
                                                   .name = "NV4XMZ",
                                                   .version = std::nullopt,
                                                   .serial_number = std::nullopt,
                                                   .firmware_vendor = "American Megatrends International, LLC.",
                                                   .firmware_version = "N.1.10A13",
                                               }));
}

TEST(Smbios, EmptyTable) {
  std::vector<std::byte> bytes;
  append(bytes, {127, 4, 0x00, 0x00, 0, 0});
  const auto t = smbios::parse(bytes, smbios::Version{.major = 3, .minor = 0});
  ASSERT_TRUE(t.has_value());
  EXPECT_EQ(internal::memory_info(*t), hwinfo::Memory{});
  EXPECT_EQ(internal::computer_identity(*t), std::nullopt);
  EXPECT_EQ(internal::mainboard_info(*t), std::nullopt);
}
