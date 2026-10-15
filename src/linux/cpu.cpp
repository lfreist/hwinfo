// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/cpu.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <ranges>
#include <string>
#include <vector>

#include "internal/core_types.h"
#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

namespace {

const std::filesystem::path sysfs_cpu = "/sys/devices/system/cpu";

Cache read_cache(const std::filesystem::path& cpu_path) {
  Cache cache;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator(cpu_path / "cache", ec)) {
    if (!entry.path().filename().string().starts_with("index")) {
      continue;
    }
    const auto level = internal::read_number_attribute<int>(entry.path() / "level");
    const auto type = internal::read_attribute(entry.path() / "type");
    const auto size = internal::read_attribute(entry.path() / "size").and_then(internal::procfs::parse_cache_size);
    if (!level || !type) {
      continue;
    }
    if (*level == 1 && *type == "Data") {
      cache.l1_data = size;
    } else if (*level == 1 && *type == "Instruction") {
      cache.l1_instruction = size;
    } else if (*level == 2) {
      cache.l2 = size;
    } else if (*level == 3) {
      cache.l3 = size;
    }
  }
  return cache;
}

std::optional<Hertz> read_khz(const std::filesystem::path& path) {
  return internal::read_number_attribute<std::uint64_t>(path).transform(
      [](const std::uint64_t khz) { return khz * FrequencyUnit::kHz; });
}

// Logical CPU -> core type rank on Intel hybrid CPUs, which register one PMU per core type (Linux 5.13).
// Empty on other CPUs.
std::map<std::uint32_t, std::uint32_t> intel_hybrid_ranks() {
  const auto pmu_cpus = [](const char* pmu) {
    return internal::read_attribute(std::filesystem::path("/sys/devices") / pmu / "cpus")
        .and_then(internal::procfs::parse_cpu_list);
  };
  std::map<std::uint32_t, std::uint32_t> ranks;
  if (const auto atom = pmu_cpus("cpu_atom"), core = pmu_cpus("cpu_core"); atom && core) {
    for (const auto cpu : *atom) {
      ranks[cpu] = 0;
    }
    for (const auto cpu : *core) {
      ranks[cpu] = 1;
    }
  }
  return ranks;
}

// Raw performance value of a logical CPU (higher = faster), for internal::assign_core_types.
std::optional<std::uint32_t> performance_rank(std::uint32_t processor,
                                              const std::map<std::uint32_t, std::uint32_t>& intel_hybrid) {
  if (!intel_hybrid.empty()) {
    const auto it = intel_hybrid.find(processor);
    return it != intel_hybrid.end() ? std::optional(it->second) : std::nullopt;
  }
  // The scheduler's capacity of the CPU, distinct per core type on arm64. 1024 everywhere on non-hybrid CPUs.
  return internal::read_number_attribute<std::uint32_t>(sysfs_cpu / std::format("cpu{}", processor) / "cpu_capacity");
}

}  // namespace

result<std::vector<Cpu>> cpus() {
  auto processors = internal::read_file("/proc/cpuinfo").and_then(internal::procfs::parse_cpuinfo);
  if (!processors) {
    return std::unexpected(processors.error());
  }
  // The topology in sysfs: /proc/cpuinfo lacks "physical id" and "core id" on some architectures (e.g. arm64).
  for (auto& p : *processors) {
    const auto topology = sysfs_cpu / std::format("cpu{}", p.processor) / "topology";
    if (const auto package = internal::read_number_attribute<std::int64_t>(topology / "physical_package_id");
        package && *package >= 0) {  // -1 if unknown
      p.physical_id = static_cast<std::uint32_t>(*package);
    }
    if (const auto core = internal::read_number_attribute<std::uint32_t>(topology / "core_id")) {
      p.core_id = *core;
    }
  }

  // socket id -> core id -> logical processors
  std::map<std::uint32_t, std::map<std::uint32_t, std::vector<const internal::procfs::CpuinfoProcessor*>>> sockets;
  for (const auto& p : *processors) {
    sockets[p.physical_id][p.core_id].push_back(&p);
  }

  const auto intel_hybrid = intel_hybrid_ranks();

  std::vector<Cpu> result;
  result.reserve(sockets.size());
  for (const auto& [socket_id, cores] : sockets) {
    const auto& first = *cores.begin()->second.front();
    Cpu cpu{.socket = socket_id, .vendor = first.vendor, .model = first.model, .flags = first.flags};
    std::vector<std::optional<std::uint32_t>> core_ranks;
    for (const auto& [core_id, threads] : cores) {
      const auto path = sysfs_cpu / std::format("cpu{}", threads.front()->processor);
      cpu.cores.push_back(Core{
          .id = core_id,
          .threads = static_cast<std::uint32_t>(threads.size()),
          .cache = read_cache(path),
          .base_frequency = read_khz(path / "cpufreq/base_frequency"),
          .max_frequency = read_khz(path / "cpufreq/cpuinfo_max_freq"),
          .logical_ids = std::ranges::to<std::vector>(
              threads | std::views::transform(&internal::procfs::CpuinfoProcessor::processor)),
      });
      std::ranges::sort(cpu.cores.back().logical_ids);
      core_ranks.push_back(performance_rank(threads.front()->processor, intel_hybrid));
      cpu.logical_cores += static_cast<std::uint32_t>(threads.size());
    }
    cpu.physical_cores = static_cast<std::uint32_t>(cpu.cores.size());
    internal::assign_core_types(cpu.cores, core_ranks);
    result.push_back(std::move(cpu));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
