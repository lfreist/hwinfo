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
#include <string_view>
#include <vector>

namespace hwinfo {

// Physical packaging of a memory module. soldered: memory chips on the mainboard (e.g. LPDDR in notebooks).
enum class MemoryFormFactor { unknown, dimm, sodimm, camm, soldered, other };

// An installed memory module (DIMM). Not available on every platform.
// Frequencies are data rates in MT/s, reported as MHz (DDR5-4800: 4800 MHz), as in the module's SMBIOS entry.
struct MemoryModule {
  std::uint32_t index = 0;
  std::optional<std::string> vendor{};
  std::optional<std::string> name{};  // position on the board, e.g. "DIMM A1", "Controller0-ChannelA-DIMM0"
  std::optional<std::string> model{};
  std::optional<std::string> serial_number{};
  std::optional<Bytes> size{};
  std::optional<Hertz> frequency{};      // configured speed
  std::optional<Hertz> max_frequency{};  // rated speed of the module
  std::optional<std::string> type{};     // e.g. "DDR4", "DDR5", "LPDDR5"
  MemoryFormFactor form_factor = MemoryFormFactor::unknown;
  std::optional<std::uint32_t> rank{};
  std::optional<std::uint32_t> data_width{};   // bits, e.g. 64
  std::optional<std::uint32_t> total_width{};  // bits including error correction; above data_width for ECC modules
  std::optional<std::uint32_t> voltage_mv{};   // configured voltage

  friend bool operator==(const MemoryModule&, const MemoryModule&) = default;
};

struct Memory {
  Bytes total{};                         // physical memory usable by the OS
  std::vector<MemoryModule> modules{};   // installed modules; empty if module information is not available
  std::optional<std::uint32_t> slots{};  // module positions on the board, including empty and soldered ones
  std::optional<Bytes> max_capacity{};   // maximum the board / memory controller supports

  friend bool operator==(const Memory&, const Memory&) = default;
};

// Physical memory of the system. See hwinfo/monitoring.h for free and available memory.
[[nodiscard]] HWINFO_API result<Memory> memory();

constexpr std::string_view to_string(MemoryFormFactor form_factor) noexcept {
  switch (form_factor) {
    case MemoryFormFactor::dimm:
      return "DIMM";
    case MemoryFormFactor::sodimm:
      return "SO-DIMM";
    case MemoryFormFactor::camm:
      return "CAMM";
    case MemoryFormFactor::soldered:
      return "soldered";
    case MemoryFormFactor::other:
      return "other";
    case MemoryFormFactor::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const Memory& memory) {
  if (memory.modules.empty()) {
    return std::format("{}", memory.total);
  }
  return std::format("{} ({} modules)", memory.total, memory.modules.size());
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::MemoryFormFactor> : hwinfo::detail::to_string_formatter<hwinfo::MemoryFormFactor> {};

template <>
struct std::formatter<hwinfo::Memory> : hwinfo::detail::to_string_formatter<hwinfo::Memory> {};
