// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

// Core type classification shared by the platform implementations. Not part of the public API.

#pragma once

#include <hwinfo/cpu.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace hwinfo::internal {

// Sets type and efficiency_class of `cores` from a raw per-core performance value as the OS reports it (Windows
// EfficiencyClass, Linux cpu_capacity, ...): higher = faster, `ranks[i]` belongs to `cores[i]`.
// efficiency_class becomes the dense rank of the value (0 = slowest), the fastest cores become performance cores and
// all others efficiency cores. Leaves the cores untouched unless every core has a value.
inline void assign_core_types(std::span<Core> cores, std::span<const std::optional<std::uint32_t>> ranks) {
  if (cores.size() != ranks.size() || std::ranges::any_of(ranks, [](const auto& r) { return !r.has_value(); })) {
    return;
  }
  std::vector<std::uint32_t> distinct;
  distinct.reserve(ranks.size());
  for (const auto& r : ranks) {
    distinct.push_back(*r);
  }
  std::ranges::sort(distinct);
  const auto [last, end] = std::ranges::unique(distinct);
  distinct.erase(last, end);

  for (std::size_t i = 0; i < cores.size(); ++i) {
    const auto rank = static_cast<std::size_t>(std::ranges::lower_bound(distinct, *ranks[i]) - distinct.begin());
    cores[i].efficiency_class = static_cast<std::uint8_t>(std::min<std::size_t>(rank, UINT8_MAX));
    cores[i].type = rank + 1 == distinct.size() ? CoreType::performance : CoreType::efficiency;
  }
}

}  // namespace hwinfo::internal
