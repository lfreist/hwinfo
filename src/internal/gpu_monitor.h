// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Sources of live GPU data for GpuSampler. Not part of the public API.

#pragma once

#include <hwinfo/gpu.h>
#include <hwinfo/monitoring.h>

#include <chrono>
#include <memory>
#include <optional>

namespace hwinfo::internal::gpu {

// One source of live data about one GPU. Created with a baseline for counter based metrics.
class StatusSource {
 public:
  virtual ~StatusSource() = default;

  // Fills the fields of `status` this source knows and that are still empty (sources are asked by priority).
  virtual void sample(GpuStatus& status) = 0;
};

// Each returns nullptr if the source does not know the GPU (library missing, other vendor, ...).
std::unique_ptr<StatusSource> nvml_source(const Gpu& gpu);
std::unique_ptr<StatusSource> level_zero_source(const Gpu& gpu);
std::unique_ptr<StatusSource> os_source(const Gpu& gpu);  // platform specific: src/<platform>/monitoring/gpu.cpp

// Whether the GPU is powered down by runtime power management, so that querying it would wake it up (platform
// specific; only detectable on Linux).
bool is_suspended(const Gpu& gpu);

template <typename T>
void fill(std::optional<T>& field, std::optional<T> value) {
  if (!field && value) {
    field = std::move(value);
  }
}

// Average rate of a monotonic counter between two readings, e.g. energy (uJ) per time (us) = W.
class CounterRate {
 public:
  // Returns (value - previous value) / (time - previous time), or nullopt for the first reading or a counter reset.
  std::optional<double> update(std::uint64_t value, std::uint64_t time) {
    std::optional<double> rate;
    if (_last && time > _last->time && value >= _last->value) {
      rate = static_cast<double>(value - _last->value) / static_cast<double>(time - _last->time);
    }
    _last = Reading{value, time};
    return rate;
  }

 private:
  struct Reading {
    std::uint64_t value;
    std::uint64_t time;
  };
  std::optional<Reading> _last;
};

// Microseconds of a steady clock, for counters without own timestamps.
inline std::uint64_t now_us() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

}  // namespace hwinfo::internal::gpu
