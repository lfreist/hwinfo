// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/cpu.h>
#include <windows.h>

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "internal/win_registry.h"
#include "internal/windows_error.h"
#include "internal/windows_power.h"
#include "internal/windows_processors.h"

namespace hwinfo {

namespace {

struct CacheEntry {
  GROUP_AFFINITY affinity{};
  BYTE level = 0;
  PROCESSOR_CACHE_TYPE type = CacheUnified;
  DWORD size = 0;
};

struct Topology {
  std::vector<std::vector<GROUP_AFFINITY>> packages;  // processor groups spanned by each package
  std::vector<GROUP_AFFINITY> cores;
  std::vector<CacheEntry> caches;
};

bool overlaps(const GROUP_AFFINITY& a, const GROUP_AFFINITY& b) { return a.Group == b.Group && (a.Mask & b.Mask) != 0; }

std::vector<GROUP_AFFINITY> group_masks(const PROCESSOR_RELATIONSHIP& processor) {
  const GROUP_AFFINITY* masks = processor.GroupMask;
  return {masks, masks + processor.GroupCount};
}

result<Topology> read_topology() {
  DWORD size = 0;
  if (GetLogicalProcessorInformationEx(RelationAll, nullptr, &size) || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
    return std::unexpected(internal::last_error("GetLogicalProcessorInformationEx"));
  }
  std::vector<std::byte> buffer(size);
  if (!GetLogicalProcessorInformationEx(
          RelationAll, reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data()), &size)) {
    return std::unexpected(internal::last_error("GetLogicalProcessorInformationEx"));
  }

  Topology topology;
  for (std::size_t offset = 0; offset < size;) {
    const auto* info = reinterpret_cast<const SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);
    if (info->Size == 0) {
      break;
    }
    switch (info->Relationship) {
      case RelationProcessorPackage:
        topology.packages.push_back(group_masks(info->Processor));
        break;
      case RelationProcessorCore:
        if (info->Processor.GroupCount > 0) {
          topology.cores.push_back(info->Processor.GroupMask[0]);
        }
        break;
      case RelationCache:
        topology.caches.push_back(CacheEntry{
            .affinity = info->Cache.GroupMask,
            .level = info->Cache.Level,
            .type = info->Cache.Type,
            .size = info->Cache.CacheSize,
        });
        break;
      default:
        break;
    }
    offset += info->Size;
  }
  return topology;
}

std::uint32_t first_processor(const GROUP_AFFINITY& affinity) {
  return internal::group_offset(affinity.Group) +
         static_cast<std::uint32_t>(std::countr_zero(static_cast<std::uint64_t>(affinity.Mask)));
}

std::wstring processor_key(std::uint32_t processor) {
  return std::format(L"HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\{}", processor);
}

Cache read_cache(const GROUP_AFFINITY& core, const std::vector<CacheEntry>& caches) {
  Cache cache;
  for (const auto& entry : caches) {
    if (!overlaps(core, entry.affinity)) {
      continue;
    }
    const Bytes size{entry.size};
    if (entry.level == 1 && entry.type == CacheData) {
      cache.l1_data = size;
    } else if (entry.level == 1 && entry.type == CacheInstruction) {
      cache.l1_instruction = size;
    } else if (entry.level == 1 && entry.type == CacheUnified) {
      cache.l1_data = size;
      cache.l1_instruction = size;
    } else if (entry.level == 2) {
      cache.l2 = size;
    } else if (entry.level == 3) {
      cache.l3 = size;
    }
  }
  return cache;
}

std::optional<Hertz> mhz(std::uint64_t value) {
  return value > 0 ? std::optional(value * FrequencyUnit::MHz) : std::nullopt;
}

// Feature flags named like the flags in Linux' /proc/cpuinfo.
// The PF_* constants are given as numbers since older Windows SDKs do not define all of them.
// Unknown features are reported as not present by older Windows versions.
std::vector<std::string> read_flags() {
  struct Feature {
    DWORD id;
    const char* name;
  };
#if defined(HWINFO_X86)
  constexpr Feature features[] = {
      {3, "mmx"},     {6, "sse"},    {10, "sse2"},   {13, "pni"},     {36, "ssse3"},    {37, "sse4_1"},
      {38, "sse4_2"}, {39, "avx"},   {40, "avx2"},   {41, "avx512f"}, {7, "3dnow"},     {8, "tsc"},
      {9, "pae"},     {12, "nx"},    {14, "cx16"},   {17, "xsave"},   {22, "fsgsbase"}, {28, "rdrand"},
      {32, "rdtscp"}, {33, "rdpid"}, {35, "mwaitx"}, {42, "erms"},
  };
#elif defined(HWINFO_ARM)
  constexpr Feature features[] = {
      {19, "asimd"}, {30, "aes"},     {30, "pmull"},   {30, "sha1"},  {30, "sha2"},
      {31, "crc32"}, {34, "atomics"}, {43, "asimddp"}, {44, "jscvt"}, {45, "lrcpc"},
  };
#else
  constexpr Feature features[] = {{0, nullptr}};
#endif
  std::vector<std::string> flags;
  for (const auto& [id, name] : features) {
    if (name != nullptr && IsProcessorFeaturePresent(id)) {
      flags.emplace_back(name);
    }
  }
  return flags;
}

}  // namespace

result<std::vector<Cpu>> cpus() {
  auto topology = read_topology();
  if (!topology) {
    return std::unexpected(topology.error());
  }
  if (topology->packages.empty()) {
    // should not happen: treat all cores as one package
    topology->packages.push_back(topology->cores);
  }
  const auto power =
      internal::processor_power_information().value_or(std::vector<internal::ProcessorPowerInformation>{});
  const auto flags = read_flags();

  std::vector<Cpu> result;
  result.reserve(topology->packages.size());
  for (const auto& package : topology->packages) {
    const auto key = processor_key(package.empty() ? 0 : first_processor(package.front()));
    Cpu cpu{
        .socket = static_cast<std::uint32_t>(result.size()),
        .vendor = internal::registry::read_string(HKEY_LOCAL_MACHINE, key, L"VendorIdentifier").value_or(""),
        .model = internal::registry::read_string(HKEY_LOCAL_MACHINE, key, L"ProcessorNameString").value_or(""),
        .flags = flags,
    };
    for (const auto& core : topology->cores) {
      if (std::ranges::none_of(package, [&](const GROUP_AFFINITY& group) { return overlaps(group, core); })) {
        continue;
      }
      auto logical_ids = internal::processors(core);
      const std::uint32_t processor = logical_ids.empty() ? 0 : logical_ids.front();
      const auto threads = static_cast<std::uint32_t>(logical_ids.size());
      cpu.cores.push_back(Core{
          .id = static_cast<std::uint32_t>(cpu.cores.size()),
          .threads = threads,
          .cache = read_cache(core, topology->caches),
          .base_frequency =
              internal::registry::read_dword(HKEY_LOCAL_MACHINE, processor_key(processor), L"~MHz").and_then(mhz),
          .max_frequency = processor < power.size() ? mhz(power[processor].max_mhz) : std::nullopt,
          .logical_ids = std::move(logical_ids),
      });
      cpu.logical_cores += threads;
    }
    cpu.physical_cores = static_cast<std::uint32_t>(cpu.cores.size());
    result.push_back(std::move(cpu));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
