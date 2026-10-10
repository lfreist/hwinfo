// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Aggregation of the Windows "GPU Engine" / "GPU Adapter Memory" performance counters (the data of Task Manager's GPU
// view).
// Pure functions on the counter instance names, testable on any platform.
// Not part of the public API.

#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hwinfo::internal::gpu_counters {

// The instance name prefix identifying an adapter: "luid_0x00000000_0x0000d1a6" (lowercase).
inline std::string luid_prefix(std::uint64_t luid) {
  return std::format("luid_0x{:08x}_0x{:08x}", static_cast<std::uint32_t>(luid >> 32),
                     static_cast<std::uint32_t>(luid & 0xffffffff));
}

inline std::string lowercase(std::string_view s) {
  std::string out(s);
  std::ranges::transform(out, out.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  return out;
}

struct EngineLoad {
  std::optional<double> utilization;        // busiest graphics / compute / copy engine, [0, 1]
  std::optional<double> video_utilization;  // busiest video engine, [0, 1]
};

/**
 * Aggregates "\GPU Engine(*)\Utilization Percentage" values of the adapter with the given LUID.
 * Instance names look like "pid_1234_luid_0x00000000_0x0000D1A6_phys_0_eng_3_engtype_VideoDecode": one instance per
 * process and engine. Like Task Manager, the processes are summed per engine and the busiest engine counts.
 */
inline EngineLoad aggregate_engines(const std::vector<std::pair<std::string, double>>& instances, std::uint64_t luid) {
  const std::string adapter = luid_prefix(luid);
  std::map<std::string, std::pair<double, bool>> engines;  // "phys_0_eng_3" -> (percent, is video)
  for (const auto& [raw_name, percent] : instances) {
    const std::string name = lowercase(raw_name);
    const auto at = name.find(adapter);
    const auto engine = name.find("_phys_");
    const auto type = name.find("_engtype_");
    if (at == std::string::npos || engine == std::string::npos || type == std::string::npos || engine > type) {
      continue;
    }
    const std::string_view engine_type = std::string_view(name).substr(type + 9);
    auto& [sum, video] = engines[name.substr(engine + 1, type - engine - 1)];
    sum += percent;
    video = engine_type.starts_with("video");
  }
  EngineLoad load;
  for (const auto& [sum, video] : engines | std::views::values) {
    auto& field = video ? load.video_utilization : load.utilization;
    field = std::max(field.value_or(0.0), std::min(sum / 100.0, 1.0));
  }
  return load;
}

// Sum of "\GPU Adapter Memory(*)\Dedicated Usage" values (bytes) of the adapter ("luid_0x..._0x..._phys_0").
inline std::optional<std::uint64_t> adapter_memory(const std::vector<std::pair<std::string, double>>& instances,
                                                   std::uint64_t luid) {
  const std::string adapter = luid_prefix(luid);
  std::optional<std::uint64_t> total;
  for (const auto& [name, bytes] : instances) {
    if (lowercase(name).starts_with(adapter)) {
      total = total.value_or(0) + static_cast<std::uint64_t>(bytes);
    }
  }
  return total;
}

}  // namespace hwinfo::internal::gpu_counters
