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

TEST(Smbios, NoMemoryDevices) {
  std::vector<std::byte> bytes;
  append(bytes, {127, 4, 0x00, 0x00, 0, 0});
  const auto t = smbios::parse(bytes, smbios::Version{.major = 3, .minor = 0});
  ASSERT_TRUE(t.has_value());
  EXPECT_TRUE(internal::memory_modules(*t).empty());
}
