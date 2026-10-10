// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/monitoring.h>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "internal/apple_cf.h"
#include "internal/gpu_monitor.h"
#include "internal/pci.h"

namespace hwinfo::internal::gpu {

namespace {

namespace cf = internal::apple;

// PCI address of the PCI device an accelerator is attached to; nullopt for Apple Silicon GPUs.
std::optional<std::string> pci_address(io_registry_entry_t accelerator) {
  for (auto current = cf::parent(accelerator); current; current = cf::parent(current.get())) {
    if (IOObjectConformsTo(current.get(), "IOPCIDevice")) {
      return cf::string_property(current.get(), CFSTR("pcidebug")).and_then(internal::parse_pcidebug);
    }
  }
  return std::nullopt;
}

template <typename T>
std::optional<T> statistic(CFTypeRef statistics, CFStringRef key) {
  return cf::to_number<T>(cf::dictionary_value(statistics, key));
}

// The "PerformanceStatistics" the GPU driver publishes on its IOAccelerator service (also read by Activity Monitor).
class AcceleratorSource final : public StatusSource {
 public:
  explicit AcceleratorSource(cf::io_ptr accelerator) : _accelerator(std::move(accelerator)) {}

  void sample(GpuStatus& status) override {
    const auto statistics = cf::property(_accelerator.get(), CFSTR("PerformanceStatistics"));
    if (!statistics) {
      return;
    }
    if (const auto percent = statistic<std::int64_t>(statistics.get(), CFSTR("Device Utilization %"))) {
      fill(status.utilization, std::optional(static_cast<double>(*percent) / 100.0));
    }
    // discrete GPUs report VRAM, Apple Silicon the unified memory in use by the GPU
    auto used = statistic<std::uint64_t>(statistics.get(), CFSTR("vramUsedBytes"));
    if (!used) {
      used = statistic<std::uint64_t>(statistics.get(), CFSTR("In use system memory"));
    }
    if (used) {
      fill(status.memory_used, std::optional(Bytes{*used}));
    }
  }

 private:
  cf::io_ptr _accelerator;
};

}  // namespace

std::unique_ptr<StatusSource> os_source(const Gpu& gpu) {
  auto accelerators = cf::matching_services("IOAccelerator");
  if (!accelerators) {
    return nullptr;
  }
  const std::optional<std::string> wanted = gpu.pci ? gpu.pci->address : std::nullopt;
  for (auto& accelerator : *accelerators) {
    const auto address = pci_address(accelerator.get());
    if ((wanted && address == wanted) || (!gpu.pci && !address)) {
      return std::make_unique<AcceleratorSource>(std::move(accelerator));
    }
  }
  return nullptr;
}

bool is_suspended(const Gpu&) { return false; }

}  // namespace hwinfo::internal::gpu

#endif  // HWINFO_APPLE
