// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/dmi.h"

#include <gtest/gtest.h>

#include <format>
#include <optional>
#include <string>

namespace internal = hwinfo::internal;
using hwinfo::ChassisType;

TEST(Dmi, KeepsRealValues) {
  EXPECT_EQ(internal::dmi_string("LENOVO"), "LENOVO");
  EXPECT_EQ(internal::dmi_string("  SCHENKER VISION (M23)\n"), "SCHENKER VISION (M23)");
  EXPECT_EQ(internal::dmi_string("NoneSuch"), "NoneSuch");
}

TEST(Dmi, DropsPlaceholders) {
  EXPECT_EQ(internal::dmi_string(std::nullopt), std::nullopt);
  EXPECT_EQ(internal::dmi_string(""), std::nullopt);
  EXPECT_EQ(internal::dmi_string("   "), std::nullopt);
  EXPECT_EQ(internal::dmi_string("To Be Filled By O.E.M."), std::nullopt);
  EXPECT_EQ(internal::dmi_string("to be filled by o.e.m."), std::nullopt);
  EXPECT_EQ(internal::dmi_string("System Product Name"), std::nullopt);
  EXPECT_EQ(internal::dmi_string("Default string "), std::nullopt);
  EXPECT_EQ(internal::dmi_string("NONE"), std::nullopt);
}

TEST(Dmi, ChassisFromSmbios) {
  static_assert(internal::chassis_from_smbios(3) == ChassisType::desktop);
  static_assert(internal::chassis_from_smbios(7) == ChassisType::desktop);
  static_assert(internal::chassis_from_smbios(10) == ChassisType::laptop);
  static_assert(internal::chassis_from_smbios(31) == ChassisType::laptop);
  static_assert(internal::chassis_from_smbios(30) == ChassisType::tablet);
  static_assert(internal::chassis_from_smbios(13) == ChassisType::all_in_one);
  static_assert(internal::chassis_from_smbios(35) == ChassisType::mini_pc);
  static_assert(internal::chassis_from_smbios(23) == ChassisType::server);
  static_assert(internal::chassis_from_smbios(2) == ChassisType::unknown);
  static_assert(internal::chassis_from_smbios(1) == ChassisType::other);
  static_assert(internal::chassis_from_smbios(0x80 | 10) == ChassisType::laptop);  // lock bit set
}

TEST(Dmi, ChassisToString) {
  EXPECT_EQ(std::format("{}", ChassisType::all_in_one), "all-in-one");
  EXPECT_EQ(std::format("{:>8}", ChassisType::laptop), "  laptop");
}
