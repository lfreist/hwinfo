// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include <gtest/gtest.h>
#include <hwinfo/error.h>

#include <cerrno>
#include <format>

using hwinfo::errc;
using hwinfo::error;

TEST(Error, OwnCodesCompareEqualToTheirCondition) {
  const error e{errc::parse_error, "/proc/cpuinfo"};
  EXPECT_EQ(e, errc::parse_error);
  EXPECT_NE(e, errc::io_error);
  EXPECT_EQ(e.context(), "/proc/cpuinfo");
}

TEST(Error, ForeignCodesMapToConditions) {
  const auto errno_error = [](int value) { return error{std::error_code(value, std::generic_category())}; };
  EXPECT_EQ(errno_error(EACCES), errc::permission_denied);
  EXPECT_EQ(errno_error(EPERM), errc::permission_denied);
  EXPECT_EQ(errno_error(ENOENT), errc::not_found);
  EXPECT_EQ(errno_error(ENOTSUP), errc::not_supported);
  EXPECT_EQ(errno_error(EIO), errc::io_error);
  // the original std::errc can still be compared
  EXPECT_EQ(errno_error(EACCES), std::errc::permission_denied);
  EXPECT_NE(errno_error(EACCES), errc::not_found);
}

TEST(Error, MessageContainsContextAndDescription) {
  const error e{std::error_code(ENOENT, std::generic_category()), "/sys/foo"};
  EXPECT_EQ(e.message(), "/sys/foo: " + std::generic_category().message(ENOENT));
  EXPECT_EQ(std::format("{}", e), e.message());
  EXPECT_EQ(std::format("{}", error{errc::not_supported}), "not supported on this platform");
}

TEST(Error, ExpectedIntegration) {
  const hwinfo::result<int> r = std::unexpected(error{errc::not_found});
  ASSERT_FALSE(r);
  EXPECT_EQ(r.error(), errc::not_found);
  EXPECT_EQ(r.value_or(42), 42);
}
