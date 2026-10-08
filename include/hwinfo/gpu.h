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

struct PciId {
  std::uint16_t vendor = 0;
  std::uint16_t device = 0;

  friend bool operator==(const PciId&, const PciId&) = default;
};

struct Gpu {
  std::uint32_t index = 0;
  std::string vendor{};
  std::string name{};
  std::optional<std::string> driver{};
  std::optional<std::string> driver_version{};
  std::optional<Bytes> dedicated_memory{};
  std::optional<Bytes> shared_memory{};
  std::optional<Hertz> frequency{};
  std::optional<std::uint32_t> cores{};
  std::optional<PciId> pci{};

  friend bool operator==(const Gpu&, const Gpu&) = default;
};

// All GPUs of the system.
[[nodiscard]] HWINFO_API result<std::vector<Gpu>> gpus();

inline std::string to_string(const Gpu& gpu) { return std::format("{} {}", gpu.vendor, gpu.name); }

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Gpu> : hwinfo::detail::to_string_formatter<hwinfo::Gpu> {};
