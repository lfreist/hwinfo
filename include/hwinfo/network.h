// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace hwinfo {

struct NetworkInterface {
  std::uint32_t index = 0;                   // OS interface index
  std::string name{};                        // e.g. "eth0", "en0", adapter name on Windows
  std::optional<std::string> description{};  // human readable adapter description
  std::optional<std::string> mac{};          // "aa:bb:cc:dd:ee:ff"
  std::vector<std::string> ipv4{};
  std::vector<std::string> ipv6{};
  bool is_up = false;
  bool is_loopback = false;

  friend bool operator==(const NetworkInterface&, const NetworkInterface&) = default;
};

[[nodiscard]] HWINFO_API result<std::vector<NetworkInterface>> network_interfaces();

inline std::string to_string(const NetworkInterface& nic) {
  return std::format("{} ({})", nic.name, nic.description.value_or(nic.is_up ? "up" : "down"));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::NetworkInterface> : hwinfo::detail::to_string_formatter<hwinfo::NetworkInterface> {};
