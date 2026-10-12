// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "internal/cim.h"

#include <gtest/gtest.h>

using namespace std::chrono;
using hwinfo::internal::cim::parse_datetime;

TEST(Cim, DatetimeToUtc) {
  EXPECT_EQ(parse_datetime("20261012083015.500000+120"), sys_days(2026y / October / 12) + 6h + 30min + 15s);
  EXPECT_EQ(parse_datetime("20261012083015.000000-300"), sys_days(2026y / October / 12) + 13h + 30min + 15s);
  EXPECT_EQ(parse_datetime("20240101000000.000000+000"), sys_days(2024y / January / 1));
  EXPECT_EQ(parse_datetime("20240101003000.000000+060"), sys_days(2023y / December / 31) + 23h + 30min);
}

TEST(Cim, InvalidDatetimes) {
  EXPECT_EQ(parse_datetime(""), std::nullopt);
  EXPECT_EQ(parse_datetime("20261012083015"), std::nullopt);
  EXPECT_EQ(parse_datetime("20261312083015.000000+000"), std::nullopt);  // month 13
  EXPECT_EQ(parse_datetime("2026101208301x.000000+000"), std::nullopt);
  EXPECT_EQ(parse_datetime("20261012083015.000000*000"), std::nullopt);
}
