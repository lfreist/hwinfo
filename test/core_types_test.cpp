// Copyright Leon Freist
// Authored by Claude (Opus 5.5)

#include "internal/core_types.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <format>
#include <optional>
#include <vector>

using hwinfo::Core;
using hwinfo::CoreType;
using hwinfo::internal::assign_core_types;

namespace {
using Ranks = std::vector<std::optional<std::uint32_t>>;
}  // namespace

TEST(CoreTypes, TwoTiers) {
  std::vector<Core> cores(4);
  assign_core_types(cores, Ranks{1, 1, 0, 0});
  EXPECT_EQ(cores[0].type, CoreType::performance);
  EXPECT_EQ(cores[1].efficiency_class, 1);
  EXPECT_EQ(cores[2].type, CoreType::efficiency);
  EXPECT_EQ(cores[3].efficiency_class, 0);
}

TEST(CoreTypes, DenseRankOfRawValues) {
  // arm64 cpu_capacity of a prime/big/LITTLE SoC
  std::vector<Core> cores(4);
  assign_core_types(cores, Ranks{1024, 870, 870, 325});
  EXPECT_EQ(cores[0].efficiency_class, 2);
  EXPECT_EQ(cores[0].type, CoreType::performance);
  EXPECT_EQ(cores[1].efficiency_class, 1);
  EXPECT_EQ(cores[1].type, CoreType::efficiency);
  EXPECT_EQ(cores[3].efficiency_class, 0);
  EXPECT_EQ(cores[3].type, CoreType::efficiency);
}

TEST(CoreTypes, NonHybridIsPerformance) {
  std::vector<Core> cores(2);
  assign_core_types(cores, Ranks{1024, 1024});
  for (const auto& core : cores) {
    EXPECT_EQ(core.type, CoreType::performance);
    EXPECT_EQ(core.efficiency_class, 0);
  }
  hwinfo::Cpu cpu{.cores = cores};
  EXPECT_FALSE(cpu.is_hybrid());
}

TEST(CoreTypes, MissingValueLeavesUnknown) {
  std::vector<Core> cores(2);
  assign_core_types(cores, Ranks{1, std::nullopt});
  for (const auto& core : cores) {
    EXPECT_EQ(core.type, CoreType::unknown);
    EXPECT_EQ(core.efficiency_class, std::nullopt);
  }
}

TEST(CoreTypes, Format) {
  EXPECT_EQ(std::format("{}", CoreType::efficiency), "efficiency");
  EXPECT_EQ(std::format("{}", CoreType::unknown), "unknown");
}
