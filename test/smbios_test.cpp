// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

#include "internal/smbios.h"

#include <gtest/gtest.h>
#include <hwinfo/ram.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <format>
#include <initializer_list>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "internal/file.h"

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

std::vector<std::byte> table_bytes() {
  std::vector<std::byte> bytes;
  using namespace std::string_view_literals;
  // BIOS (type 0): vendor, version, segment, release date, ROM size, characteristics
  structure(bytes, 0, 0x00, {1, 2, 0x00, 0xf0, 3, 0xff, 0, 0, 0, 0, 0, 0, 0, 0},
            "American Megatrends International, LLC.\0N.1.10A13\0"
            "05/14/2024\0"sv);
  // System (type 1): manufacturer, product, version, serial, UUID, wake-up type, SKU, family
  // (UUID 86f17d41-e257-95c0-a2aa-8c148ab96b97, first three fields little endian since SMBIOS 2.6)
  structure(bytes, 1, 0x01, {1,    2,    3,    4,    0x41, 0x7d, 0xf1, 0x86, 0x57, 0xe2, 0xc0, 0x95,
                             0xa2, 0xaa, 0x8c, 0x14, 0x8a, 0xb9, 0x6b, 0x97, 0x06, 5,    6},
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
  memory_device(bytes, {.handle = 0x23, .array = 0x1001, .size_mib = 16, .type = 0x09, .strings = "BIOS\0"sv});
  append(bytes, {127, 4, 0x24, 0x00, 0, 0});
  return bytes;
}

smbios::Table table() {
  auto parsed = smbios::parse(table_bytes(), smbios::Version{.major = 2, .minor = 8});
  EXPECT_TRUE(parsed.has_value());
  return *std::move(parsed);
}

// Sets the checksum byte at `at` so that the bytes of `range` sum to 0.
void fix_checksum(std::vector<std::byte>& bytes, std::size_t first, std::size_t count, std::size_t at) {
  bytes[at] = std::byte{0};
  unsigned sum = 0;
  for (std::size_t i = first; i < first + count; ++i) {
    sum += std::to_integer<unsigned>(bytes[i]);
  }
  bytes[at] = static_cast<std::byte>((0x100 - (sum & 0xff)) & 0xff);
}

// SMBIOS 3.0 entry point (_SM3_, 0x18 bytes) for a table of at most `length` bytes.
std::vector<std::byte> entry_point_v3(std::uint32_t length) {
  std::vector<std::byte> bytes;
  append(bytes, "_SM3_");
  append(bytes, {0, 0x18, 3, 0, 0, 1, 0});  // checksum, length, version 3.0.0, revision 1, reserved
  append(bytes, {length & 0xff, (length >> 8) & 0xff, (length >> 16) & 0xff, length >> 24});
  append(bytes, {0x00, 0x10, 0x0f, 0, 0, 0, 0, 0});  // table address
  fix_checksum(bytes, 0, 0x18, 0x05);
  return bytes;
}

// SMBIOS 2.8 entry point (_SM_ with _DMI_, 0x1f bytes) for a table of exactly `length` bytes.
std::vector<std::byte> entry_point_v2(std::uint16_t length, unsigned structures) {
  std::vector<std::byte> bytes;
  append(bytes, "_SM_");
  append(bytes, {0, 0x1f, 2, 8, 0, 0, 0, 0, 0, 0, 0, 0});  // checksum, length, version, max size, revision, area
  append(bytes, "_DMI_");
  append(bytes, {0, length & 0xffu, static_cast<unsigned>(length) >> 8, 0x00, 0x10, 0x0f, 0, structures & 0xff,
                 structures >> 8, 0x28});
  fix_checksum(bytes, 0x10, 0x0f, 0x15);
  fix_checksum(bytes, 0, 0x1f, 0x04);
  return bytes;
}

void expect_parse_error(const hwinfo::result<hwinfo::Memory>& memory) {
  ASSERT_FALSE(memory.has_value());
  EXPECT_EQ(memory.error(), hwinfo::errc::parse_error);
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
  EXPECT_EQ(computer->uuid, internal::parse_uuid("86f17d41-e257-95c0-a2aa-8c148ab96b97"));
  EXPECT_EQ(computer->chassis, hwinfo::ChassisType::laptop);
  EXPECT_EQ(computer->chassis_code, 10);  // lock bit masked
  EXPECT_EQ(computer->boot_time, std::nullopt);
}

TEST(Smbios, MainboardInfo) {
  EXPECT_EQ(
      internal::mainboard_info(table()),
      (hwinfo::Mainboard{
          .vendor = "NB02",
          .name = "NV4XMZ",
          .version = std::nullopt,
          .serial_number = std::nullopt,
          .firmware_vendor = "American Megatrends International, LLC.",
          .firmware_version = "N.1.10A13",
          .firmware_date = std::chrono::year_month_day{std::chrono::year(2024), std::chrono::May, std::chrono::day(14)},
      }));
}

TEST(Smbios, VmFlag) {
  EXPECT_FALSE(internal::smbios_vm_flag(table()));  // BIOS information without extension bytes (before 2.4)

  const auto bios = [](unsigned extension2) {
    using namespace std::string_view_literals;
    std::vector<std::byte> bytes;
    structure(bytes, 0, 0x00, {1, 2, 0x00, 0xe8, 0, 0x00, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x01, extension2},
              "SeaBIOS\0"
              "1.16.3\0"sv);
    append(bytes, {127, 4, 0x01, 0x00, 0, 0});
    auto parsed = smbios::parse(bytes, smbios::Version{.major = 2, .minor = 8});
    EXPECT_TRUE(parsed.has_value());
    return *std::move(parsed);
  };
  EXPECT_TRUE(internal::smbios_vm_flag(bios(0x1c)));  // targeted content distribution, UEFI, VM (as QEMU)
  EXPECT_FALSE(internal::smbios_vm_flag(bios(0x0c)));
}

TEST(Smbios, SystemTableIsReadOnce) {
  // succeeds or fails depending on the platform and permissions, but always the same object
  EXPECT_EQ(&internal::system_table(), &internal::system_table());
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

#ifdef HWINFO_HAS_RAM

TEST(Smbios, MemoryFromBytes) {
  const auto bytes = table_bytes();
  const auto expected = internal::memory_info(table());

  const auto v3 = hwinfo::memory_from_smbios(entry_point_v3(static_cast<std::uint32_t>(bytes.size())), bytes);
  ASSERT_TRUE(v3.has_value()) << v3.error().message();
  EXPECT_EQ(*v3, expected);
  EXPECT_EQ(v3->total, hwinfo::Bytes{});
  EXPECT_EQ(v3->modules.size(), 2u);

  // 3.x states a maximum length
  EXPECT_EQ(hwinfo::memory_from_smbios(entry_point_v3(0x1000), bytes), expected);

  const auto v2 = hwinfo::memory_from_smbios(entry_point_v2(static_cast<std::uint16_t>(bytes.size()), 11), bytes);
  ASSERT_TRUE(v2.has_value()) << v2.error().message();
  EXPECT_EQ(*v2, expected);
}

TEST(Smbios, MemoryFromBytesInvalidEntryPoint) {
  const auto bytes = table_bytes();
  const auto length = static_cast<std::uint32_t>(bytes.size());

  auto anchor = entry_point_v3(length);
  anchor[3] = std::byte{'X'};
  expect_parse_error(hwinfo::memory_from_smbios(anchor, bytes));

  auto checksum = entry_point_v3(length);
  checksum[0x0c] = static_cast<std::byte>(std::to_integer<unsigned>(checksum[0x0c]) + 1);
  expect_parse_error(hwinfo::memory_from_smbios(checksum, bytes));

  auto dmi_checksum = entry_point_v2(static_cast<std::uint16_t>(length), 11);
  dmi_checksum[0x15] = static_cast<std::byte>(std::to_integer<unsigned>(dmi_checksum[0x15]) + 1);
  expect_parse_error(hwinfo::memory_from_smbios(dmi_checksum, bytes));

  const auto truncated = entry_point_v3(length);
  expect_parse_error(hwinfo::memory_from_smbios(std::span(truncated).first(0x10), bytes));
  expect_parse_error(hwinfo::memory_from_smbios({}, bytes));
}

TEST(Smbios, MemoryFromBytesMalformedTable) {
  const auto bytes = table_bytes();
  const auto ep = entry_point_v3(0x1000);

  expect_parse_error(hwinfo::memory_from_smbios(ep, {}));
  // cut inside a memory device, inside a header, and between the two NULs ending a string set
  for (const std::size_t cut : {bytes.size() - 20, bytes.size() - 4, bytes.size() - 1}) {
    expect_parse_error(hwinfo::memory_from_smbios(ep, std::span(bytes).first(cut)));
  }
  // cut at a structure boundary: complete structures, but no type 127
  expect_parse_error(hwinfo::memory_from_smbios(ep, std::span(bytes).first(bytes.size() - 6)));

  // longer than the entry point allows
  expect_parse_error(hwinfo::memory_from_smbios(entry_point_v3(static_cast<std::uint32_t>(bytes.size() - 1)), bytes));

  // a structure with a length below the header size
  auto short_length = bytes;
  short_length[1] = std::byte{3};
  expect_parse_error(hwinfo::memory_from_smbios(ep, short_length));
}

TEST(Smbios, MemoryFromBytesV2Length) {
  // 2.x: a table without type 127 must have exactly the stated length
  auto bytes = table_bytes();
  bytes.resize(bytes.size() - 6);
  const auto length = static_cast<std::uint16_t>(bytes.size());
  EXPECT_EQ(hwinfo::memory_from_smbios(entry_point_v2(length, 10), bytes), internal::memory_info(table()));
  expect_parse_error(hwinfo::memory_from_smbios(entry_point_v2(static_cast<std::uint16_t>(length + 40), 10), bytes));

  // Linux trims the table after type 127, so a shorter table that ends with it is complete
  const auto full = table_bytes();
  EXPECT_TRUE(
      hwinfo::memory_from_smbios(entry_point_v2(static_cast<std::uint16_t>(full.size() + 40), 11), full).has_value());
}

TEST(Smbios, MemoryFromBytesShortMemoryStructures) {
  using namespace std::string_view_literals;
  const auto check = [](unsigned type, std::initializer_list<unsigned> fields) {
    std::vector<std::byte> bytes;
    memory_array(bytes, 0x1000, 0x03, 16 * 1024 * 1024, 2);
    memory_device(bytes, {.handle = 0x20, .size_mib = 0x2000, .strings = "DIMM 0\0BANK 0\0A\0B\0C\0"sv});
    structure(bytes, type, 0x30, fields, "");
    append(bytes, {127, 4, 0x31, 0x00, 0, 0});

    expect_parse_error(hwinfo::memory_from_smbios(entry_point_v3(0x1000), bytes));
    // memory() stays best effort: the short structure is skipped
    const auto parsed = smbios::parse(bytes, smbios::Version{.major = 3, .minor = 0});
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(internal::memory_info(*parsed).modules.size(), 1u);
  };
  check(17, {0x00, 0x10, 0xfe, 0xff});  // memory device below 0x15 bytes
  check(16, {0x03, 0x03});              // memory array below 0x0f bytes
}

#ifdef __linux__
TEST(Smbios, MemoryFromSystemBytes) {
  const auto entry_point = internal::read_file("/sys/firmware/dmi/tables/smbios_entry_point");
  const auto dmi = internal::read_file("/sys/firmware/dmi/tables/DMI");
  if (!entry_point || !dmi) {
    GTEST_SKIP() << "SMBIOS tables not readable (root only)";
  }
  auto expected = hwinfo::memory();
  ASSERT_TRUE(expected.has_value());
  expected->total = hwinfo::Bytes{};
  EXPECT_EQ(hwinfo::memory_from_smbios(std::as_bytes(std::span(*entry_point)), std::as_bytes(std::span(*dmi))),
            *expected);
}
#endif

#endif  // HWINFO_HAS_RAM
