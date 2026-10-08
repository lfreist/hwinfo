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

// An installed memory module (DIMM). Not available on every platform.
struct MemoryModule {
  std::uint32_t index = 0;
  std::optional<std::string> vendor{};
  std::optional<std::string> name{};
  std::optional<std::string> model{};
  std::optional<std::string> serial_number{};
  std::optional<Bytes> size{};
  std::optional<Hertz> frequency{};

  friend bool operator==(const MemoryModule&, const MemoryModule&) = default;
};

struct Memory {
  Bytes total{};                        // physical memory usable by the OS
  std::vector<MemoryModule> modules{};  // empty if module information is not available

  friend bool operator==(const Memory&, const Memory&) = default;
};

// Physical memory of the system. See hwinfo/monitoring.h for free and available memory.
[[nodiscard]] HWINFO_API result<Memory> memory();

inline std::string to_string(const Memory& memory) {
  if (memory.modules.empty()) {
    return std::format("{}", memory.total);
  }
  return std::format("{} ({} modules)", memory.total, memory.modules.size());
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Memory> : hwinfo::detail::to_string_formatter<hwinfo::Memory> {};
