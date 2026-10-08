// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Internal sysctl helpers returning hwinfo::result (macOS). Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/error.h>
#include <sys/sysctl.h>
#include <sys/types.h>  // before sys/sysctl.h

#include <algorithm>
#include <array>
#include <cerrno>
#include <concepts>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <string>
#include <system_error>
#include <type_traits>

#include "strings.h"

namespace hwinfo::internal {

inline error sysctl_error(const char* name, int err) {
  return error{std::error_code(err != 0 ? err : EIO, std::generic_category()), std::format("sysctl {}", name)};
}

// A string value, e.g. sysctl_string("kern.osrelease").
// The terminating NUL is removed.
inline result<std::string> sysctl_string(const char* name) {
  for (int attempt = 0; attempt < 3; ++attempt) {
    std::size_t size = 0;
    if (sysctlbyname(name, nullptr, &size, nullptr, 0) != 0) {
      return std::unexpected(sysctl_error(name, errno));
    }
    std::string buffer(size, '\0');
    if (sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0) {
      if (errno == ENOMEM) {
        continue;
      }
      return std::unexpected(sysctl_error(name, errno));
    }
    buffer.resize(std::min(size, buffer.size()));
    if (const auto nul = buffer.find('\0'); nul != std::string::npos) {
      buffer.resize(nul);
    }
    return buffer;
  }
  return std::unexpected(sysctl_error(name, ENOMEM));
}

// A string value: std::nullopt if it does not exist, is empty or can't be read.
inline std::optional<std::string> sysctl_attribute(const char* name) {
  const auto value = sysctl_string(name);
  return value ? non_empty(*value) : std::nullopt;
}

// Reads a `Stored` from the raw sysctl buffer and converts it to T.
template <typename T, typename Stored>
T load_sysctl_value(const unsigned char* bytes) {
  Stored value{};
  std::memcpy(&value, bytes, sizeof(Stored));
  return static_cast<T>(value);
}

// An integer value, e.g. sysctl_value<std::uint64_t>("hw.memsize").
// Values stored with a different width than T (sysctls are 32 or 64 bit) are converted.
template <typename T>
  requires std::integral<T>
result<T> sysctl_value(const char* name) {
  std::array<unsigned char, 8> buffer{};
  std::size_t size = buffer.size();
  if (sysctlbyname(name, buffer.data(), &size, nullptr, 0) != 0) {
    return std::unexpected(sysctl_error(name, errno));
  }
  constexpr bool is_signed = std::is_signed_v<T>;
  switch (size) {
    case 1:
      return is_signed ? load_sysctl_value<T, std::int8_t>(buffer.data())
                       : load_sysctl_value<T, std::uint8_t>(buffer.data());
    case 2:
      return is_signed ? load_sysctl_value<T, std::int16_t>(buffer.data())
                       : load_sysctl_value<T, std::uint16_t>(buffer.data());
    case 4:
      return is_signed ? load_sysctl_value<T, std::int32_t>(buffer.data())
                       : load_sysctl_value<T, std::uint32_t>(buffer.data());
    case 8:
      return is_signed ? load_sysctl_value<T, std::int64_t>(buffer.data())
                       : load_sysctl_value<T, std::uint64_t>(buffer.data());
    default:
      return std::unexpected(error{errc::parse_error, std::format("sysctl {}: unexpected size {}", name, size)});
  }
}

// An integer value: std::nullopt if it does not exist or can't be read.
template <typename T>
  requires std::integral<T>
std::optional<T> sysctl_number(const char* name) {
  const auto value = sysctl_value<T>(name);
  return value ? std::optional<T>(*value) : std::nullopt;
}

}  // namespace hwinfo::internal

#endif  // HWINFO_APPLE
