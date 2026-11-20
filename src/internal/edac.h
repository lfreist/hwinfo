// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Memory modules from the Linux EDAC subsystem (/sys/devices/system/edac/mc), readable without root. Not part of the
// public API.

#pragma once

#include <hwinfo/ram.h>
#include <hwinfo/units.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "file.h"
#include "strings.h"

namespace hwinfo::internal::edac {

struct MemoryKind {
  std::optional<std::string> type{};  // as named by SMBIOS, e.g. "DDR4", "LPDDR4"
  MemoryFormFactor form_factor = MemoryFormFactor::unknown;
};

// The kernel's edac_mem_types[] string of dimm_mem_type, e.g. "Unbuffered-DDR4", "Registered-DDR5",
// "Low-Power-DDR4-RAM", "Load-Reduced-DDR4-RAM".
inline MemoryKind memory_kind(std::string_view text) {
  text = trim(text);
  // Registered, load-reduced and fully buffered memory only exists as (server) DIMMs. Unbuffered can be either.
  const bool dimm =
      text.starts_with("Registered-") || text.starts_with("Load-Reduced-") || text.starts_with("FullyBuffered-");
  constexpr std::array<std::pair<std::string_view, std::string_view>, 13> types{{
      {"Low-Power-DDR3", "LPDDR3"},
      {"Low-Power-DDR4", "LPDDR4"},
      {"Low-Power-DDR5", "LPDDR5"},
      {"-DDR5", "DDR5"},
      {"-DDR4", "DDR4"},
      {"-DDR3", "DDR3"},
      {"-DDR2", "DDR2"},
      {"-DDR", "DDR"},
      {"-SDR", "SDRAM"},
      {"High-bandwidth-memory-Gen2", "HBM2"},
      {"High-bandwidth-memory-Gen3", "HBM3"},
      {"RMBS", "RDRAM"},
      {"Wide-IO-2", "Wide I/O 2"},
  }};
  for (const auto& [pattern, name] : types) {
    const auto at = text.find(pattern);
    // "-DDR" must not match "-DDR4": the pattern ends the text or is followed by "-RAM"
    if (at != std::string_view::npos) {
      const auto rest = text.substr(at + pattern.size());
      if (rest.empty() || rest == "-RAM") {
        return {std::string(name), dimm ? MemoryFormFactor::dimm : MemoryFormFactor::unknown};
      }
    }
  }
  return {std::nullopt, dimm ? MemoryFormFactor::dimm : MemoryFormFactor::unknown};
}

// Memory modules of all memory controllers under `edac_mc` (/sys/devices/system/edac/mc): their "dimm<N>" (or, for
// chip select based drivers, "rank<N>") directories with a size. std::nullopt if there is none (no EDAC driver loaded,
// or it reports no populated module). EDAC exposes size, type and label only.
inline std::optional<std::vector<MemoryModule>> memory_modules(const std::filesystem::path& edac_mc) {
  namespace fs = std::filesystem;
  std::error_code ec;
  std::vector<fs::path> dimms;
  for (const auto& controller : fs::directory_iterator(edac_mc, ec)) {
    if (!controller.path().filename().string().starts_with("mc")) {
      continue;
    }
    std::error_code inner;
    for (const auto& entry : fs::directory_iterator(controller.path(), inner)) {
      const auto name = entry.path().filename().string();
      if (name.starts_with("dimm") || name.starts_with("rank")) {
        dimms.push_back(entry.path());
      }
    }
  }
  // mc0/dimm0, mc0/dimm2, mc1/dimm0, ...; numerically for more than 10 entries
  std::ranges::sort(dimms, [](const fs::path& a, const fs::path& b) {
    const auto key = [](const fs::path& path) {
      const auto number = [](std::string s, std::size_t prefix) {
        return parse<std::uint32_t>(std::string_view(s).substr(prefix)).value_or(0);
      };
      return std::pair(number(path.parent_path().filename().string(), 2), number(path.filename().string(), 4));
    };
    return key(a) < key(b);
  });

  std::vector<MemoryModule> modules;
  for (const auto& dimm : dimms) {
    const auto mib = read_number_attribute<std::uint64_t>(dimm / "size");
    if (!mib || *mib == 0) {
      continue;  // empty slot
    }
    const auto kind = memory_kind(read_attribute(dimm / "dimm_mem_type").value_or(""));
    modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(modules.size()),
        .name = read_attribute(dimm / "dimm_label"),
        .size = *mib * ByteUnit::MiB,
        .type = kind.type,
        .form_factor = kind.form_factor,
    });
  }
  if (modules.empty()) {
    return std::nullopt;
  }
  return modules;
}

}  // namespace hwinfo::internal::edac
