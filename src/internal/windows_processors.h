// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// System wide numbering of logical processors across processor groups. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <windows.h>

#include <bit>
#include <cstdint>
#include <vector>

namespace hwinfo::internal {

// System wide number of the first logical processor of `group`.
inline std::uint32_t group_offset(WORD group) {
  std::uint32_t number = 0;
  for (WORD g = 0; g < group; ++g) {
    number += GetActiveProcessorCount(g);
  }
  return number;
}

// System wide numbers of the logical processors in `affinity`, as used by the registry and the power information
// (and reported as Core::logical_ids).
inline std::vector<std::uint32_t> processors(const GROUP_AFFINITY& affinity) {
  const std::uint32_t offset = group_offset(affinity.Group);
  std::vector<std::uint32_t> numbers;
  for (auto mask = static_cast<std::uint64_t>(affinity.Mask); mask != 0; mask &= mask - 1) {
    numbers.push_back(offset + static_cast<std::uint32_t>(std::countr_zero(mask)));
  }
  return numbers;
}

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
