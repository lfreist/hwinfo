// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/monitoring.h>

#include <memory>
#include <vector>

#include "internal/gpu_monitor.h"

namespace hwinfo {

struct GpuSampler::State {
  Gpu gpu;
  GpuQuery query;
  bool connected = false;
  std::vector<std::unique_ptr<internal::gpu::StatusSource>> sources;  // by priority

  // Opens the sources and takes the baseline for counter based rates. Deferred while the GPU is suspended: opening
  // a vendor library's device handle wakes the GPU up.
  void connect() {
    connected = true;
    const auto add = [&](std::unique_ptr<internal::gpu::StatusSource> source) {
      if (source) {
        sources.push_back(std::move(source));
      }
    };
#ifndef HWINFO_GPU_NO_BACKENDS
    if (query.nvml) {
      add(internal::gpu::nvml_source(gpu));
    }
    if (query.level_zero) {
      add(internal::gpu::level_zero_source(gpu));
    }
#endif
    add(internal::gpu::os_source(gpu));
    GpuStatus baseline;
    for (const auto& source : sources) {
      source->sample(baseline);
    }
  }
};

GpuSampler::GpuSampler(const Gpu& gpu, const GpuQuery& query) : _state(std::make_unique<State>()) {
  _state->gpu = gpu;
  _state->query = query;
  if (!internal::gpu::is_suspended(gpu)) {
    _state->connect();
  }
}

GpuSampler::GpuSampler(GpuSampler&&) noexcept = default;
GpuSampler& GpuSampler::operator=(GpuSampler&&) noexcept = default;
GpuSampler::~GpuSampler() = default;

result<GpuStatus> GpuSampler::sample() {
  if (!_state) {
    return std::unexpected(error{errc::not_supported, "moved-from GpuSampler"});
  }
  GpuStatus status;
  if (internal::gpu::is_suspended(_state->gpu)) {
    status.suspended = true;
    status.utilization = 0.0;
    return status;
  }
  if (!_state->connected) {
    _state->connect();
  }
  if (_state->sources.empty()) {
    return std::unexpected(error{errc::not_supported, "no source of GPU status data for this GPU"});
  }
  for (const auto& source : _state->sources) {
    source->sample(status);
  }
  return status;
}

}  // namespace hwinfo
