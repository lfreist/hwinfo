// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/monitoring.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "internal/file.h"
#include "internal/gpu_monitor.h"
#include "internal/pci.h"

namespace hwinfo::internal::gpu {

namespace {

namespace fs = std::filesystem;

std::optional<fs::path> pci_path(const Gpu& gpu) {
  if (!gpu.pci || !gpu.pci->address) {
    return std::nullopt;
  }
  fs::path path = fs::path("/sys/bus/pci/devices") / *gpu.pci->address;
  std::error_code ec;
  return fs::exists(path, ec) ? std::optional(path) : std::nullopt;
}

template <typename T>
std::optional<T> number(const fs::path& path) {
  return internal::read_number_attribute<T>(path);
}

// sysfs files of the kernel driver: amdgpu, hwmon (amdgpu, xe, i915 dGPUs, nouveau), i915 / xe frequency files and
// the PCIe link state.
class SysfsSource final : public StatusSource {
 public:
  explicit SysfsSource(fs::path device) : _device(std::move(device)) {
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(_device / "hwmon", ec)) {
      _hwmon = entry.path();
      break;
    }
    for (const auto& entry : fs::directory_iterator(_device / "drm", ec)) {
      if (entry.path().filename().string().starts_with("card")) {
        _card = entry.path();
        break;
      }
    }
  }

  void sample(GpuStatus& status) override {
    // amdgpu
    if (const auto busy = number<std::uint32_t>(_device / "gpu_busy_percent")) {
      fill(status.utilization, std::optional(*busy / 100.0));
    }
    if (const auto busy = number<std::uint32_t>(_device / "mem_busy_percent")) {
      fill(status.memory_utilization, std::optional(*busy / 100.0));
    }
    if (const auto used = number<std::uint64_t>(_device / "mem_info_vram_used")) {
      fill(status.memory_used, std::optional(Bytes{*used}));
    }
    if (const auto total = number<std::uint64_t>(_device / "mem_info_vram_total")) {
      fill(status.memory_total, std::optional(Bytes{*total}));
    }
    if (_hwmon) {
      sample_hwmon(*_hwmon, status);
    }
    if (_card) {
      // i915: gt_act_freq_mhz, xe: device/tile0/gt0/freq0/act_freq
      auto mhz = number<std::uint64_t>(*_card / "gt_act_freq_mhz");
      if (!mhz) {
        mhz = number<std::uint64_t>(_device / "tile0" / "gt0" / "freq0" / "act_freq");
      }
      if (mhz && *mhz > 0) {
        fill(status.frequency, std::optional(*mhz * FrequencyUnit::MHz));
      }
    }
    const auto generation = internal::read_attribute(_device / "current_link_speed").and_then(pcie_generation);
    const auto width = number<std::uint32_t>(_device / "current_link_width");
    if (generation && width && *width > 0) {
      fill(status.pcie_link, std::optional(PcieLink{.generation = *generation, .width = *width}));
    }
  }

 private:
  void sample_hwmon(const fs::path& hwmon, GpuStatus& status) {
    if (const auto millidegrees = number<std::int64_t>(hwmon / "temp1_input")) {
      fill(status.temperature, std::optional(static_cast<double>(*millidegrees) / 1000.0));
    }
    // power: averaged / instant reading in uW, or an energy counter in uJ (xe, i915)
    auto microwatts = number<std::uint64_t>(hwmon / "power1_average");
    if (!microwatts) {
      microwatts = number<std::uint64_t>(hwmon / "power1_input");
    }
    if (microwatts) {
      fill(status.power, std::optional(Power{*microwatts}));
    } else if (const auto energy = number<std::uint64_t>(hwmon / "energy1_input")) {
      if (const auto watts = _energy.update(*energy, now_us())) {
        fill(status.power, std::optional(Power{static_cast<std::uint64_t>(*watts * 1e6)}));
      }
    }
    if (const auto pwm = number<std::uint32_t>(hwmon / "pwm1")) {
      const auto max = number<std::uint32_t>(hwmon / "pwm1_max").value_or(255);
      if (max > 0) {
        fill(status.fan_speed, std::optional(static_cast<double>(*pwm) / max));
      }
    }
    // amdgpu: freq1 = shader clock, freq2 = memory clock, in Hz
    if (const auto hz = number<std::uint64_t>(hwmon / "freq1_input")) {
      fill(status.frequency, std::optional(Hertz{*hz}));
    }
    if (const auto hz = number<std::uint64_t>(hwmon / "freq2_input")) {
      fill(status.memory_frequency, std::optional(Hertz{*hz}));
    }
  }

  fs::path _device;
  std::optional<fs::path> _hwmon;
  std::optional<fs::path> _card;
  CounterRate _energy;
};

}  // namespace

std::unique_ptr<StatusSource> os_source(const Gpu& gpu) {
  const auto path = pci_path(gpu);
  return path ? std::make_unique<SysfsSource>(*path) : nullptr;
}

bool is_suspended(const Gpu& gpu) {
  const auto path = pci_path(gpu);
  return path && internal::read_attribute(*path / "power" / "runtime_status") == "suspended";
}

}  // namespace hwinfo::internal::gpu

#endif  // HWINFO_UNIX
