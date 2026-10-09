// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Dynamic system state (utilization, free memory, battery charge, ...) and a periodic Monitor.
//
// Each function is implemented by the library of its component (CpuSampler / cpu_frequencies: hwinfo_cpu,
// memory_usage: hwinfo_ram, battery_status: hwinfo_battery).

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>
#include <hwinfo/units.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

namespace hwinfo {

// ----- CPU ----------------------------------------------------------------------------------------------------------

struct CpuLoad {
  double total = 0;  // average over all logical cores, [0, 1]
  // per logical core, [0, 1], indexed by OS CPU number (see Core::logical_ids). Offline cores report 0.
  std::vector<double> per_thread{};
};

namespace detail {
struct CpuTicks {
  std::uint64_t busy = 0;
  std::uint64_t total = 0;
};
}  // namespace detail

/**
 * Measures CPU utilization between two points in time.
 *
 *   hwinfo::CpuSampler sampler;               // takes the baseline
 *   std::this_thread::sleep_for(500ms);
 *   auto load = sampler.sample();             // utilization of the last 500 ms; new baseline
 *
 * A sampler holds no global state: independent samplers can be used concurrently.
 */
class HWINFO_API CpuSampler {
 public:
  CpuSampler();

  // Utilization since construction or the previous call to sample().
  [[nodiscard]] result<CpuLoad> sample();

 private:
  std::vector<detail::CpuTicks> _last;  // [0]: all cores, [1 + i]: logical core i
};

// Current clock rate per logical core, indexed by OS CPU number (see Core::logical_ids). Offline cores report 0 Hz.
[[nodiscard]] HWINFO_API result<std::vector<Hertz>> cpu_frequencies();

// ----- Memory -------------------------------------------------------------------------------------------------------

struct MemoryUsage {
  Bytes total{};
  Bytes free{};       // not used at all
  Bytes available{};  // available for new allocations without swapping (includes reclaimable caches)
};

[[nodiscard]] HWINFO_API result<MemoryUsage> memory_usage();

// ----- Disk ---------------------------------------------------------------------------------------------------------

struct DiskSpace {
  Bytes capacity{};
  Bytes free{};
  Bytes available{};  // free space available to the calling user
};

// Space of the filesystem containing `path` (e.g. one of Disk::mount_points).
[[nodiscard]] inline result<DiskSpace> disk_space(const std::filesystem::path& path) {
  std::error_code ec;
  const auto info = std::filesystem::space(path, ec);
  if (ec) {
    return std::unexpected(error{ec, path.string()});
  }
  return DiskSpace{{info.capacity}, {info.free}, {info.available}};
}

// ----- Battery ------------------------------------------------------------------------------------------------------

enum class BatteryState { unknown, charging, discharging, full, not_charging };

struct BatteryStatus {
  BatteryState state = BatteryState::unknown;
  std::optional<double> charge{};  // [0, 1]
};

// Status of the battery with the given Battery::index.
[[nodiscard]] HWINFO_API result<BatteryStatus> battery_status(std::uint32_t index);

constexpr std::string_view to_string(BatteryState state) noexcept {
  switch (state) {
    case BatteryState::charging:
      return "charging";
    case BatteryState::discharging:
      return "discharging";
    case BatteryState::full:
      return "full";
    case BatteryState::not_charging:
      return "not charging";
    case BatteryState::unknown:
      break;
  }
  return "unknown";
}

// ----- Monitor ------------------------------------------------------------------------------------------------------

/**
 * Calls `fetch` every `interval` on a background thread and passes the result to `on_data`.
 * Starts on construction; stops when stop() is called or the Monitor is destroyed.
 *
 *   hwinfo::Monitor monitor{1s, [s = hwinfo::CpuSampler{}]() mutable { return s.sample(); },
 *                           [](const hwinfo::result<hwinfo::CpuLoad>& load) { ... }};
 */
template <typename T>
class Monitor {
 public:
  using Fetch = std::function<T()>;
  using Callback = std::function<void(const T&)>;

  Monitor(std::chrono::milliseconds interval, Fetch fetch, Callback on_data)
      : _interval(interval), _fetch(std::move(fetch)), _on_data(std::move(on_data)) {
    _thread = std::jthread([this](std::stop_token token) { run(token); });
  }

  Monitor(const Monitor&) = delete;
  Monitor& operator=(const Monitor&) = delete;
  Monitor(Monitor&&) = delete;
  Monitor& operator=(Monitor&&) = delete;

  ~Monitor() { stop(); }

  // Stops the monitor and waits for a running callback to finish. When called from the callback itself, the monitor
  // stops after the callback returns.
  void stop() {
    _thread.request_stop();
    if (_thread.joinable() && _thread.get_id() != std::this_thread::get_id()) {
      _thread.join();
    }
  }

  [[nodiscard]] bool running() const noexcept { return _thread.joinable(); }

 private:
  void run(std::stop_token token) {
    while (!token.stop_requested()) {
      _on_data(_fetch());
      std::unique_lock lock(_mutex);
      _cv.wait_for(lock, token, _interval, [] { return false; });
    }
  }

  std::chrono::milliseconds _interval;
  Fetch _fetch;
  Callback _on_data;
  std::mutex _mutex;
  std::condition_variable_any _cv;
  std::jthread _thread;  // last member: started after and joined before all others are destroyed
};

template <typename F, typename C>
Monitor(std::chrono::milliseconds, F, C) -> Monitor<std::invoke_result_t<F&>>;

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::BatteryState> : hwinfo::detail::to_string_formatter<hwinfo::BatteryState> {};
