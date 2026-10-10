// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Intel specific knowledge shared by the Level Zero and OpenCL backends.

#include <format>
#include <string_view>

#include "internal/gpu_backend.h"

namespace hwinfo::internal::gpu {

namespace {

// Graphics IP versions: https://github.com/intel/compute-runtime (shared/source/helpers/hw_ip_version.h) and the
// Intel graphics programmer's reference manuals.
constexpr std::string_view architecture(std::uint32_t major, std::uint32_t minor) noexcept {
  switch (major) {
    case 9:
      return "Gen9";
    case 11:
      return "Gen11";
    case 12:
      if (minor >= 70) {
        return "Xe-LPG";  // Meteor Lake, Arrow Lake
      }
      if (minor == 60) {
        return "Xe-HPC";  // Ponte Vecchio
      }
      if (minor >= 50) {
        return "Xe-HPG";  // Alchemist (Arc A-series)
      }
      return "Xe-LP";  // Tiger Lake, Alder Lake, Raptor Lake, DG1
    case 20:
      return minor >= 4 ? "Xe2-LPG" : "Xe2-HPG";  // Lunar Lake / Battlemage (Arc B-series)
    case 30:
      return "Xe3-LPG";  // Panther Lake
    default:
      return {};
  }
}

}  // namespace

void apply_intel_ip_version(Gpu& gpu, std::uint32_t ip_version) {
  if (ip_version == 0) {
    return;
  }
  const std::uint32_t major = ip_version >> 22;
  const std::uint32_t minor = (ip_version >> 14) & 0xff;
  const std::uint32_t revision = ip_version & 0x3fff;
  gpu.compute_capability = std::format("{}.{}.{}", major, minor, revision);
  if (const auto name = architecture(major, minor); !name.empty()) {
    gpu.architecture = std::string(name);
  }
}

}  // namespace hwinfo::internal::gpu
