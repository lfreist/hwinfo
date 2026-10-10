// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// NVIDIA specific knowledge shared by the NVML, CUDA and OpenCL backends, and identifier parsing.

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>

#include "internal/gpu_backend.h"

namespace hwinfo::internal::gpu {

namespace {

struct Generation {
  std::string_view architecture;
  int cores_per_sm;
};

// https://docs.nvidia.com/cuda/cuda-c-programming-guide/#compute-capabilities
constexpr Generation generation(int major, int minor) noexcept {
  switch (major) {
    case 3:
      return {"Kepler", 192};
    case 5:
      return {"Maxwell", 128};
    case 6:
      return {"Pascal", minor == 0 ? 64 : 128};
    case 7:
      return minor == 5 ? Generation{"Turing", 64} : Generation{"Volta", 64};
    case 8:
      if (minor == 0) {
        return {"Ampere", 64};
      }
      return minor == 9 ? Generation{"Ada Lovelace", 128} : Generation{"Ampere", 128};
    case 9:
      return {"Hopper", 128};
    case 10:
    case 11:
    case 12:
      return {"Blackwell", 128};
    default:
      return {{}, 0};
  }
}

}  // namespace

void apply_compute_capability(Gpu& gpu, int major, int minor) {
  gpu.compute_capability = std::format("{}.{}", major, minor);
  const auto [architecture, cores_per_sm] = generation(major, minor);
  if (!architecture.empty()) {
    gpu.architecture = std::string(architecture);
  }
  if (cores_per_sm > 0 && gpu.compute_units) {
    gpu.cores = *gpu.compute_units * static_cast<std::uint32_t>(cores_per_sm);
  }
}

std::string cuda_version(int version) { return std::format("{}.{}", version / 1000, version % 1000 / 10); }

std::optional<GpuUuid> parse_uuid(std::string_view s) {
  if (s.starts_with("GPU-")) {
    s.remove_prefix(4);
  }
  GpuUuid uuid;
  std::size_t digits = 0;
  for (const char c : s) {
    if (c == '-') {
      continue;
    }
    int value = 0;
    if (c >= '0' && c <= '9') {
      value = c - '0';
    } else if (c >= 'a' && c <= 'f') {
      value = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
      value = c - 'A' + 10;
    } else {
      return std::nullopt;
    }
    if (digits == 32) {
      return std::nullopt;
    }
    uuid.bytes[digits / 2] = static_cast<std::uint8_t>(uuid.bytes[digits / 2] << 4 | value);
    ++digits;
  }
  return digits == 32 ? std::optional(uuid) : std::nullopt;
}

}  // namespace hwinfo::internal::gpu
