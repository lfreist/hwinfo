// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/strings.h"

#include <gtest/gtest.h>

#include <ranges>
#include <string>
#include <vector>

namespace internal = hwinfo::internal;
using namespace std::string_view_literals;

namespace {
template <typename R>
std::vector<std::string_view> collect(R&& range) {
  return std::forward<R>(range) | std::ranges::to<std::vector<std::string_view>>();
}
}  // namespace

TEST(Strings, Trim) {
  EXPECT_EQ(internal::trim("  a b \t\r\n"), "a b");
  EXPECT_EQ(internal::trim("abc"), "abc");
  EXPECT_EQ(internal::trim(" \n\t "), "");
  EXPECT_EQ(internal::trim(""), "");
}

TEST(Strings, Unquote) {
  EXPECT_EQ(internal::unquote("\"Ubuntu\""), "Ubuntu");
  EXPECT_EQ(internal::unquote("'x'"), "x");
  EXPECT_EQ(internal::unquote("\"unbalanced"), "\"unbalanced");
  EXPECT_EQ(internal::unquote("\""), "\"");
}

TEST(Strings, Split) {
  EXPECT_EQ(collect(internal::split("a,b,,c", ',')), (std::vector{"a"sv, "b"sv, ""sv, "c"sv}));
  EXPECT_EQ(collect(internal::split("a::b", "::"sv)), (std::vector{"a"sv, "b"sv}));
  EXPECT_EQ(collect(internal::lines("l1\nl2")), (std::vector{"l1"sv, "l2"sv}));
  EXPECT_EQ(collect(internal::words("  fpu  vme\tde ")), (std::vector{"fpu"sv, "vme"sv, "de"sv}));
}

TEST(Strings, SplitKeyValue) {
  const auto kv = internal::split_key_value("model name\t: Intel(R) Core(TM): i7", ':');
  ASSERT_TRUE(kv);
  EXPECT_EQ(kv->first, "model name");
  EXPECT_EQ(kv->second, "Intel(R) Core(TM): i7");
  EXPECT_FALSE(internal::split_key_value("no delimiter", ':'));
}

TEST(Strings, Parse) {
  EXPECT_EQ(internal::parse<int>(" 42\n"), 42);
  EXPECT_EQ(internal::parse<std::uint16_t>("0x10de", 16), 0x10de);
  EXPECT_EQ(internal::parse<std::uint16_t>("8086", 16), 0x8086);
  EXPECT_DOUBLE_EQ(internal::parse<double>("2400.000").value(), 2400.0);
  EXPECT_EQ(internal::parse<int>("12abc").error(), hwinfo::errc::parse_error);
  EXPECT_EQ(internal::parse<int>("").error(), hwinfo::errc::parse_error);
  EXPECT_EQ(internal::parse<std::uint8_t>("300").error(), hwinfo::errc::parse_error);
}

TEST(Strings, NonEmptyAndJoin) {
  EXPECT_EQ(internal::non_empty("  x "), "x");
  EXPECT_EQ(internal::non_empty(" \n"), std::nullopt);
  EXPECT_EQ(internal::join(std::vector<std::string>{"a", "b", "c"}, ", "), "a, b, c");
  EXPECT_EQ(internal::join(std::vector<std::string>{}, ", "), "");
}

TEST(Strings, EqualsIgnoreCase) {
  static_assert(internal::equals_ignore_case("Default String", "default string"));
  static_assert(!internal::equals_ignore_case("abc", "abd"));
  static_assert(!internal::equals_ignore_case("abc", "ab"));
  static_assert(internal::to_lower('Q') == 'q' && internal::to_lower('1') == '1');
}
