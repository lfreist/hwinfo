// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

#include "internal/uuid.h"

#include <gtest/gtest.h>

#include <format>
#include <optional>

using hwinfo::Uuid;
using hwinfo::internal::parse_uuid;
using hwinfo::internal::system_uuid;

TEST(Uuid, Parse) {
  const auto uuid = parse_uuid("86f17d41-e257-95c0-a2aa-8c148ab96b97");
  ASSERT_TRUE(uuid.has_value());
  EXPECT_EQ(uuid->bytes.front(), 0x86);
  EXPECT_EQ(uuid->bytes.back(), 0x97);
  EXPECT_EQ(std::format("{}", *uuid), "86f17d41-e257-95c0-a2aa-8c148ab96b97");
  EXPECT_EQ(parse_uuid("86F17D41E25795C0A2AA8C148AB96B97"), uuid);
  EXPECT_EQ(parse_uuid("86f17d41"), std::nullopt);
  EXPECT_EQ(parse_uuid("86f17d41-e257-95c0-a2aa-8c148ab96b97-00"), std::nullopt);
  EXPECT_EQ(parse_uuid("{86f17d41-e257-95c0-a2aa-8c148ab96b97}"), std::nullopt);
}

TEST(Uuid, SystemUuidPlaceholders) {
  EXPECT_EQ(system_uuid(std::nullopt), std::nullopt);
  EXPECT_EQ(system_uuid(Uuid{}), std::nullopt);
  EXPECT_EQ(system_uuid(parse_uuid("ffffffff-ffff-ffff-ffff-ffffffffffff")), std::nullopt);
  EXPECT_EQ(system_uuid(parse_uuid("03000200-0400-0500-0006-000700080009")), std::nullopt);
  const auto real = parse_uuid("4c4c4544-0042-4410-8051-b4c04f4e3732");
  EXPECT_EQ(system_uuid(real), real);
}
