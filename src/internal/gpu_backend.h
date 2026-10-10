// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// GPU information from runtime-loaded vendor / compute libraries ("backends") and its merging into the GPU list of the
// operating system.
// Not part of the public API.

#pragma once

#include <hwinfo/gpu.h>

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo::internal::gpu {

// What one backend found out about the GPUs it sees. Each device is a partially filled Gpu:
// - pci (address / vendor_id / device_id, 0 = unknown), uuid and luid identify the device;
// - every other optional field and `type` (unknown = not known) are information to merge;
// - name is only set by vendor libraries, whose names are the marketing names ("NVIDIA GeForce RTX 4070 Laptop GPU").
using BackendResult = std::vector<Gpu>;

/**
 * Merges backend results into the GPUs enumerated by the operating system. `results` is ordered from highest to lowest
 * priority: a field known by several backends is taken from the first one, and backends override the OS values.
 *
 * Backend devices are matched to OS GPUs by PCI address, LUID or UUID; failing that by PCI vendor and device id (or by
 * vendor alone) when exactly one GPU fits. Never by name. Unmatched backend devices are dropped, so a backend can
 * neither duplicate a GPU nor add one the OS does not know.
 */
void merge(std::vector<Gpu>& gpus, std::span<const BackendResult> results);

// Queries the backends selected by `query`, ordered by priority (vendor libraries first, OpenCL last).
std::vector<BackendResult> query_backends(const GpuQuery& query);

// query_backends() + merge().
inline void enrich(std::vector<Gpu>& gpus, const GpuQuery& query) {
  if (!gpus.empty()) {
    const auto results = query_backends(query);
    merge(gpus, results);
  }
}

BackendResult nvml_devices();
BackendResult cuda_devices();
BackendResult level_zero_devices();
BackendResult opencl_devices();

// Fills the fields of a GPU from its CUDA compute capability: architecture name and CUDA cores per SM.
void apply_compute_capability(Gpu& gpu, int major, int minor);

// Fills the fields of an Intel GPU from its graphics IP version (major << 22 | minor << 14 | revision, as reported by
// Level Zero and OpenCL): compute capability "12.70.4" and architecture ("Xe-LPG").
void apply_intel_ip_version(Gpu& gpu, std::uint32_t ip_version);

// "12.8" from a CUDA version number (1000 * major + 10 * minor, e.g. 12080).
std::string cuda_version(int version);

// Parses a UUID of 32 hex digits, optionally with dashes and an NVML "GPU-" prefix
// ("GPU-86f17d41-e257-95c0-a2aa-8c148ab96b97").
std::optional<GpuUuid> parse_uuid(std::string_view s);

}  // namespace hwinfo::internal::gpu
