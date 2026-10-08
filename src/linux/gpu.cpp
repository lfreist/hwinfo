// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/gpu.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <string>
#include <vector>

#include "internal/file.h"
#include "internal/pci.h"
#include "pci.ids.h"

#ifdef USE_OCL
#include "opencl/device.h"
#endif

namespace hwinfo {

namespace {

namespace fs = std::filesystem;

std::string_view pci_database() { return {reinterpret_cast<const char*>(pci_ids), pci_ids_size}; }

// "card0", "card1", ... but not connectors like "card0-DP-1"
std::optional<std::uint32_t> card_number(std::string_view name) {
  if (!name.starts_with("card")) {
    return std::nullopt;
  }
  auto number = internal::parse<std::uint32_t>(name.substr(4));
  return number ? std::optional(*number) : std::nullopt;
}

#ifdef USE_OCL
void add_opencl_info(std::vector<Gpu>& gpus) {
  for (auto* cl_gpu : opencl_::DeviceManager::get_list<opencl_::Filter::GPU>()) {
    for (auto& gpu : gpus) {
      if (!cl_gpu->name().contains(gpu.name)) {
        continue;
      }
      gpu.driver_version = cl_gpu->driver_version();
      gpu.frequency = cl_gpu->clock_frequency_MHz() * FrequencyUnit::MHz;
      gpu.cores = static_cast<std::uint32_t>(cl_gpu->cores());
      if (!gpu.dedicated_memory) {
        gpu.dedicated_memory = Bytes{cl_gpu->memory_Bytes()};
      }
    }
  }
}
#endif

}  // namespace

result<std::vector<Gpu>> gpus() {
  const fs::path drm = "/sys/class/drm";
  std::error_code ec;
  fs::directory_iterator it(drm, ec);
  if (ec == std::errc::no_such_file_or_directory) {
    return std::vector<Gpu>{};  // no DRM driver loaded
  }
  if (ec) {
    return std::unexpected(error{ec, drm.string()});
  }
  std::vector<std::pair<std::uint32_t, fs::path>> cards;
  for (const auto& entry : it) {
    if (const auto number = card_number(entry.path().filename().string())) {
      cards.emplace_back(*number, entry.path());
    }
  }
  std::ranges::sort(cards);

  std::vector<Gpu> result;
  for (const auto& card : cards | std::views::values) {
    const fs::path device = card / "device";
    const auto vendor_id = internal::read_number_attribute<std::uint16_t>(device / "vendor", 16);
    const auto device_id = internal::read_number_attribute<std::uint16_t>(device / "device", 16);
    if (!vendor_id || !device_id) {
      continue;
    }
    const auto names = internal::lookup_pci(pci_database(), *vendor_id, *device_id);

    Gpu gpu{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = names.vendor.value_or(std::format("{:#06x}", *vendor_id)),
        .name = names.device.value_or(std::format("{:#06x}", *device_id)),
        .pci = PciId{*vendor_id, *device_id},
    };
    if (const auto driver = fs::read_symlink(device / "driver", ec); !ec) {
      gpu.driver = driver.filename().string();
      gpu.driver_version = internal::read_attribute(fs::path("/sys/module") / *gpu.driver / "version");
    }
    // amdgpu
    if (const auto vram = internal::read_number_attribute<std::uint64_t>(device / "mem_info_vram_total")) {
      gpu.dedicated_memory = Bytes{*vram};
    }
    if (const auto gtt = internal::read_number_attribute<std::uint64_t>(device / "mem_info_gtt_total")) {
      gpu.shared_memory = Bytes{*gtt};
    }
    // i915 / xe
    if (const auto mhz = internal::read_number_attribute<std::uint64_t>(card / "gt_max_freq_mhz")) {
      gpu.frequency = *mhz * FrequencyUnit::MHz;
    }
    result.push_back(std::move(gpu));
  }

#ifdef USE_OCL
  add_opencl_info(result);
#endif
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
