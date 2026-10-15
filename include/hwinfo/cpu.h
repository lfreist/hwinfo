// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>
#include <hwinfo/units.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo {

struct Cache {
  std::optional<Bytes> l1_data{};
  std::optional<Bytes> l1_instruction{};
  std::optional<Bytes> l2{};
  std::optional<Bytes> l3{};

  friend bool operator==(const Cache&, const Cache&) = default;
};

// Kind of core on a hybrid CPU (Intel P-/E-cores, Apple Silicon performance/efficiency clusters, Arm big.LITTLE).
// Cores of a non-hybrid CPU are all performance cores.
enum class CoreType : std::uint8_t { unknown, performance, efficiency };

// A physical core.
struct Core {
  // Topology id of the core within its socket.
  // Stable across runs, but not necessarily contiguous (Linux reports the hardware core id) and not usable for thread
  // affinity: use logical_ids for that.
  std::uint32_t id = 0;
  std::uint32_t threads = 1;  // hardware threads (logical cores) of this core; > 1 means SMT
  Cache cache{};
  std::optional<Hertz> base_frequency{};
  std::optional<Hertz> max_frequency{};
  // OS numbers of this core's logical processors, ascending.
  // These are the numbers thread affinity APIs take (sched_setaffinity / CPU_SET on Linux) and the indices of
  // CpuLoad::per_thread and cpu_frequencies().
  // On Windows they are numbered system wide across processor groups: with more than 64 logical processors, map them
  // to (group, bit) for SetThreadGroupAffinity.
  // Empty on macOS, which has no thread affinity API.
  std::vector<std::uint32_t> logical_ids{};
  // performance for the fastest cores of the CPU, efficiency for all slower ones. unknown if the OS does not tell.
  CoreType type = CoreType::unknown;
  // Relative performance rank within the CPU: 0 = slowest, higher = faster, equal for all cores of a non-hybrid CPU.
  // Distinguishes more than two tiers, e.g. Intel low power E-cores or Arm prime/big/LITTLE cores.
  std::optional<std::uint8_t> efficiency_class{};

  friend bool operator==(const Core&, const Core&) = default;
};

// A physical CPU package (socket).
struct Cpu {
  std::uint32_t socket = 0;
  std::string vendor{};
  std::string model{};
  std::uint32_t physical_cores = 0;
  std::uint32_t logical_cores = 0;
  std::vector<std::string> flags{};  // ISA extensions / feature flags, e.g. "avx2", "neon"
  std::vector<Core> cores{};

  [[nodiscard]] bool has_flag(std::string_view flag) const { return std::ranges::find(flags, flag) != flags.end(); }
  // Whether the CPU mixes core types, e.g. P- and E-cores.
  [[nodiscard]] bool is_hybrid() const {
    return std::ranges::any_of(cores, [](const Core& core) { return core.type == CoreType::efficiency; });
  }

  friend bool operator==(const Cpu&, const Cpu&) = default;
};

// All CPU packages of the system.
[[nodiscard]] HWINFO_API result<std::vector<Cpu>> cpus();

constexpr std::string_view to_string(CoreType type) noexcept {
  switch (type) {
    case CoreType::performance:
      return "performance";
    case CoreType::efficiency:
      return "efficiency";
    case CoreType::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const Cpu& cpu) {
  return std::format("{} {} ({} cores, {} threads)", cpu.vendor, cpu.model, cpu.physical_cores, cpu.logical_cores);
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::CoreType> : hwinfo::detail::to_string_formatter<hwinfo::CoreType> {};

template <>
struct std::formatter<hwinfo::Cpu> : hwinfo::detail::to_string_formatter<hwinfo::Cpu> {};
