// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>
#include <hwinfo/units.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

namespace hwinfo {

// Resources the calling process is restricted to, e.g. by a container, a systemd unit, a batch scheduler or a Windows
// job object. Within such limits, cpus() and memory() still report the hardware of the host.
struct ResourceLimits {
  std::optional<double> cpu_quota{};          // CPU time quota in logical cores, e.g. 1.5; std::nullopt: no quota
  std::optional<Bytes> memory{};              // std::nullopt: no limit
  std::vector<std::uint32_t> allowed_cpus{};  // logical cores the process may run on (see Core::logical_ids)

  friend bool operator==(const ResourceLimits&, const ResourceLimits&) = default;
};

[[nodiscard]] HWINFO_API result<ResourceLimits> resource_limits();

// e.g. "1.50 cores, 4.00 GiB memory, 8 allowed CPUs"
inline std::string to_string(const ResourceLimits& limits) {
  return std::format("{} cores, {} memory, {} allowed CPUs",
                     limits.cpu_quota ? std::format("{:.2f}", *limits.cpu_quota) : std::string("unlimited"),
                     limits.memory ? std::format("{}", *limits.memory) : std::string("unlimited"),
                     limits.allowed_cpus.size());
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::ResourceLimits> : hwinfo::detail::to_string_formatter<hwinfo::ResourceLimits> {};
