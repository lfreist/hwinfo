// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

#include "internal/smbios.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
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

// SMBIOS 2.7 memory device (type 17, 0x22 bytes). Strings: 1 device locator, 2 bank locator, 3 manufacturer,
// 4 serial number, 5 part number. `strings` holds them NUL-separated, followed by the terminating NUL.
void memory_device(std::vector<std::byte>& out, unsigned handle, unsigned size_mib, unsigned speed,
                   unsigned configured_speed, std::string_view strings) {
  append(out, {17, 0x22, handle & 0xff, handle >> 8});
  append(out, {0x00, 0x10, 0xfe, 0xff, 64, 0, 64, 0, size_mib & 0xff, size_mib >> 8, 0x0d, 0x00});
  append(out, {1, 2, 0x22, 0x80, 0x00, speed & 0xff, speed >> 8, 3, 4, 0, 5, 0x01, 0, 0, 0, 0});
  append(out, {configured_speed & 0xff, configured_speed >> 8});
  append(out, strings);
  append(out, {0});
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
  memory_device(bytes, 0x20, 0x2000, 4800, 4400,
                "DIMM 0\0BANK 0\0Micron Technology\0"
                "33A83711\0MTC4C10163S1SC48BA1 \0"sv);
  memory_device(bytes, 0x21, 0, 0, 0, "DIMM 1\0BANK 1\0Not Specified\0Not Specified\0Not Specified\0"sv);
  memory_device(bytes, 0x22, 0xffff, 3200, 0, "Not Specified\0BANK 2\0To Be Filled By O.E.M.\0 \0Not Specified\0"sv);
  append(bytes, {127, 4, 0x23, 0x00, 0, 0});
  auto parsed = smbios::parse(bytes, smbios::Version{.major = 2, .minor = 7});
  EXPECT_TRUE(parsed.has_value());
  return *std::move(parsed);
}

}  // namespace

TEST(Smbios, MemoryModules) {
  const auto t = table();
  const auto modules = internal::memory_modules(t);
  ASSERT_EQ(modules.size(), 2u);  // the empty slot is skipped

  EXPECT_EQ(modules[0], (hwinfo::MemoryModule{
                            .index = 0,
                            .vendor = "Micron Technology",
                            .name = "DIMM 0",
                            .model = "MTC4C10163S1SC48BA1",
                            .serial_number = "33A83711",
                            .size = 8_GiB,
                            .frequency = 4400_MHz,
                        }));

  // Unknown size, placeholders and a missing configured speed
  EXPECT_EQ(modules[1], (hwinfo::MemoryModule{
                            .index = 1,
                            .vendor = std::nullopt,
                            .name = "BANK 2",
                            .model = std::nullopt,
                            .serial_number = std::nullopt,
                            .size = std::nullopt,
                            .frequency = 3200_MHz,
                        }));
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
  EXPECT_TRUE(internal::memory_modules(*t).empty());
  EXPECT_EQ(internal::computer_identity(*t), std::nullopt);
  EXPECT_EQ(internal::mainboard_info(*t), std::nullopt);
}
