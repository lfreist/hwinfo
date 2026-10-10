// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/gpu.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/amdgpu.h"
#include "internal/file.h"
#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/pci.h"
#include "pci.ids.h"

#if __has_include(<drm/amdgpu_drm.h>)
#include <drm/amdgpu_drm.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#define HWINFO_HAS_AMDGPU_DRM
#endif

namespace hwinfo {

namespace {

namespace fs = std::filesystem;

std::string_view pci_database() { return {reinterpret_cast<const char*>(pci_ids), pci_ids_size}; }

// Framebuffers set up by the firmware: no GPU behind them.
constexpr std::array firmware_framebuffer_drivers = {
    std::string_view("simpledrm"), std::string_view("simple-framebuffer"),
    std::string_view("efidrm"),    std::string_view("vesadrm"),
    std::string_view("ofdrm"),
};

// Display adapters of virtual machines, also those not attached via PCI (e.g. virtio-mmio).
constexpr std::array virtual_drivers = {
    std::string_view("virtio_gpu"), std::string_view("vmwgfx"),      std::string_view("qxl"),
    std::string_view("bochs-drm"),  std::string_view("bochs"),       std::string_view("vboxvideo"),
    std::string_view("hyperv_drm"), std::string_view("cirrus-qemu"), std::string_view("vkms"),
};

// "card0", "card1", ... but not connectors like "card0-DP-1"
std::optional<std::uint32_t> card_number(std::string_view name) {
  if (!name.starts_with("card")) {
    return std::nullopt;
  }
  auto number = internal::parse<std::uint32_t>(name.substr(4));
  return number ? std::optional(*number) : std::nullopt;
}

// Name of the target of a sysfs symlink like "driver" or "subsystem".
std::optional<std::string> link_name(const fs::path& link) {
  std::error_code ec;
  const auto target = fs::read_symlink(link, ec);
  return ec ? std::nullopt : std::optional(target.filename().string());
}

// "PCI_SLOT_NAME=0000:01:00.0" in the device's uevent.
std::optional<std::string> pci_address(const fs::path& device) {
  const auto uevent = internal::read_file(device / "uevent");
  if (!uevent) {
    return std::nullopt;
  }
  for (const auto line : internal::lines(*uevent)) {
    if (const auto kv = internal::split_key_value(line, '='); kv && kv->first == "PCI_SLOT_NAME") {
      return std::string(kv->second);
    }
  }
  return std::nullopt;
}

// The PCI device of a DRM device: the device itself or, for virtio-gpu, the virtio PCI device it is attached to.
std::optional<fs::path> pci_device(const fs::path& device) {
  const auto subsystem = link_name(device / "subsystem");
  if (subsystem == "pci") {
    return device;
  }
  if (subsystem == "virtio" && link_name(device / ".." / "subsystem") == "pci") {
    return device / "..";
  }
  return std::nullopt;
}

// Vendor names of devicetree "compatible" prefixes of GPU IP.
std::string_view devicetree_vendor(std::string_view prefix) {
  constexpr std::array<std::pair<std::string_view, std::string_view>, 13> vendors{{
      {"allwinner", "Allwinner"},
      {"amlogic", "Amlogic"},
      {"apple", "Apple"},
      {"arm", "ARM"},
      {"brcm", "Broadcom"},
      {"fsl", "NXP"},
      {"img", "Imagination Technologies"},
      {"mediatek", "MediaTek"},
      {"nvidia", "NVIDIA"},
      {"qcom", "Qualcomm"},
      {"rockchip", "Rockchip"},
      {"samsung", "Samsung"},
      {"vivante", "Vivante"},
  }};
  const auto it = std::ranges::find(vendors, prefix, &std::pair<std::string_view, std::string_view>::first);
  return it != vendors.end() ? it->second : prefix;
}

// Vendor and model of a non PCI GPU from its devicetree node: "compatible" lists "vendor,model" entries, most specific
// first, separated by '\0'. The last, most generic one names the GPU IP, e.g. "arm,mali-valhall-csf".
void read_devicetree_names(const fs::path& device, Gpu& gpu) {
  const auto compatible = internal::read_file(device / "of_node" / "compatible");
  if (!compatible) {
    return;
  }
  std::string_view last;
  for (const auto entry : internal::split(*compatible, '\0')) {
    if (!entry.empty()) {
      last = entry;
    }
  }
  if (const auto comma = last.find(','); comma != std::string_view::npos) {
    gpu.vendor = devicetree_vendor(last.substr(0, comma));
    gpu.name = last.substr(comma + 1);
  } else {
    gpu.name = last;
  }
}

#ifdef HWINFO_HAS_AMDGPU_DRM
// Whether an amdgpu device is an APU (integrated into the CPU), asked from the driver via its render node. Render
// nodes are accessible to unprivileged users of the "render" group / with a logind seat.
std::optional<bool> amdgpu_is_apu(const fs::path& device) {
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(device / "drm", ec)) {
    const auto name = entry.path().filename().string();
    if (!name.starts_with("renderD")) {
      continue;
    }
    const int fd = open(("/dev/dri/" + name).c_str(), O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
      return std::nullopt;
    }
    drm_amdgpu_info_device info{};
    drm_amdgpu_info request{};
    request.return_pointer = reinterpret_cast<std::uintptr_t>(&info);
    request.return_size = sizeof(info);
    request.query = AMDGPU_INFO_DEV_INFO;
    const int status = ioctl(fd, DRM_IOCTL_AMDGPU_INFO, &request);
    close(fd);
    if (status != 0) {
      return std::nullopt;
    }
    return (info.ids_flags & AMDGPU_IDS_FLAGS_FUSION) != 0;
  }
  return std::nullopt;
}
#endif

std::optional<PcieLink> max_pcie_link(const fs::path& pci) {
  if (internal::read_attribute(pci / "power" / "runtime_status") == "suspended") {
    return std::nullopt;
  }
  std::optional<PcieLink> link;
  for (const auto& path : {pci, pci / ".."}) {
    const auto generation = internal::read_attribute(path / "max_link_speed").and_then(internal::pcie_generation);
    const auto width = internal::read_number_attribute<std::uint32_t>(path / "max_link_width");
    if (!generation || !width || *width == 0) {
      break;
    }
    link = link ? PcieLink{std::min(link->generation, *generation), std::min(link->width, *width)}
                : PcieLink{*generation, *width};
  }
  return link;
}

void read_amdgpu(const fs::path& device, Gpu& gpu) {
  if (const auto vbios = internal::read_attribute(device / "vbios_version")) {
    gpu.vbios_version = vbios;
  }
  if (const auto serial = internal::read_attribute(device / "serial_number"); serial && !serial->empty()) {
    gpu.serial = serial;
  }
  std::error_code ec;
  for (const auto& entry : fs::directory_iterator(device / "hwmon", ec)) {
    if (const auto cap = internal::read_number_attribute<std::uint64_t>(entry.path() / "power1_cap_default");
        cap && *cap > 0) {
      gpu.power_limit = Power{*cap};  // microwatts
    }
  }
  if (!gpu.pci || !gpu.pci->address) {
    return;
  }
  const auto& address = *gpu.pci->address;  // "dddd:bb:dd.f"
  const auto domain = internal::parse<std::uint32_t>(address.substr(0, 4), 16);
  const auto bus = internal::pci_bus(address);
  const auto slot = internal::parse<std::uint32_t>(address.substr(8, 2), 16);
  const auto function = internal::parse<std::uint32_t>(address.substr(11, 1), 16);
  if (!domain || !bus || !slot || !function) {
    return;
  }
  for (const auto& node : fs::directory_iterator("/sys/class/kfd/kfd/topology/nodes", ec)) {
    const auto content = internal::read_file(node.path() / "properties");
    if (!content) {
      continue;
    }
    const auto properties = internal::amdgpu::parse_properties(*content);
    const auto get = [&](std::string_view key) -> std::optional<std::uint64_t> {
      const auto it = properties.find(key);
      return it != properties.end() ? std::optional(it->second) : std::nullopt;
    };
    if (get("location_id") != internal::amdgpu::kfd_location_id(*bus, *slot, *function) ||
        get("domain").value_or(0) != *domain || get("simd_count").value_or(0) == 0) {
      continue;  // another GPU, or a CPU node
    }
    if (const auto version = get("gfx_target_version"); version && *version > 0) {
      const auto v = static_cast<std::uint32_t>(*version);
      gpu.compute_capability = internal::amdgpu::gfx_target(v);
      if (const auto arch = internal::amdgpu::architecture(v / 10000, v / 100 % 100, v % 100); !arch.empty()) {
        gpu.architecture = std::string(arch);
      }
    }
    const auto simds = get("simd_count");
    const auto simds_per_cu = get("simd_per_cu");
    if (simds && simds_per_cu && *simds_per_cu > 0) {
      gpu.compute_units = static_cast<std::uint32_t>(*simds / *simds_per_cu);
      gpu.cores = *gpu.compute_units * 64;  // GCN: 4 x SIMD16, RDNA: 2 x SIMD32 per compute unit
    }
    if (const auto mhz = get("max_engine_clk_fcompute"); mhz && *mhz > 0) {
      gpu.max_frequency = *mhz * FrequencyUnit::MHz;
    }
    if (const auto banks = internal::read_file(node.path() / "mem_banks" / "0" / "properties")) {
      const auto memory = internal::amdgpu::parse_properties(*banks);
      if (const auto it = memory.find("width"); it != memory.end() && it->second > 0) {
        gpu.memory_bus_width = static_cast<std::uint32_t>(it->second);
      }
      if (const auto it = memory.find("mem_clk_max"); it != memory.end() && it->second > 0) {
        gpu.max_memory_frequency = it->second * FrequencyUnit::MHz;
      }
    }
    return;
  }
}

GpuType classify(const Gpu& gpu, const fs::path& device, const std::optional<std::string>& subsystem) {
  if (gpu.driver && std::ranges::contains(virtual_drivers, *gpu.driver)) {
    return GpuType::virtualized;
  }
  if (!gpu.pci) {
    return subsystem == "platform" ? GpuType::integrated : GpuType::unknown;
  }
#ifdef HWINFO_HAS_AMDGPU_DRM
  if (gpu.pci->vendor_id == internal::pci_vendor::amd && gpu.driver == "amdgpu") {
    if (const auto apu = amdgpu_is_apu(device)) {
      return *apu ? GpuType::integrated : GpuType::discrete;
    }
  }
#else
  static_cast<void>(device);
#endif
  return internal::classify_pci_gpu(*gpu.pci);
}

}  // namespace

result<std::vector<Gpu>> gpus(const GpuQuery& query) {
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
    const auto driver = link_name(device / "driver");
    if (driver && std::ranges::contains(firmware_framebuffer_drivers, *driver)) {
      continue;
    }
    Gpu gpu{
        .index = static_cast<std::uint32_t>(result.size()),
        .driver = driver,
    };
    if (driver) {
      gpu.driver_version = internal::read_attribute(fs::path("/sys/module") / *driver / "version");
    }

    const auto pci = pci_device(device);
    const auto vendor_id =
        pci ? internal::read_number_attribute<std::uint16_t>(*pci / "vendor", 16) : std::optional<std::uint16_t>{};
    const auto device_id =
        pci ? internal::read_number_attribute<std::uint16_t>(*pci / "device", 16) : std::optional<std::uint16_t>{};
    if (vendor_id && device_id) {
      const auto names = internal::lookup_pci(pci_database(), *vendor_id, *device_id);
      gpu.vendor = names.vendor.value_or(std::format("{:#06x}", *vendor_id));
      gpu.name = names.device.value_or(std::format("{:#06x}", *device_id));
      gpu.pci = PciDevice{
          .vendor_id = *vendor_id,
          .device_id = *device_id,
          .address = pci_address(*pci),
          .max_link = max_pcie_link(*pci),
      };
    } else {
      read_devicetree_names(device, gpu);
    }
    if (gpu.name.empty()) {
      gpu.name = driver.value_or(card.filename().string());
    }
    gpu.type = classify(gpu, device, link_name(device / "subsystem"));
    gpu.unified_memory = internal::unified_memory(gpu.type);

    if (driver == "amdgpu") {
      read_amdgpu(device, gpu);
    }
    if (const auto vram = internal::read_number_attribute<std::uint64_t>(device / "mem_info_vram_total")) {
      gpu.dedicated_memory = Bytes{*vram};
    }
    if (const auto gtt = internal::read_number_attribute<std::uint64_t>(device / "mem_info_gtt_total")) {
      gpu.shared_memory = Bytes{*gtt};
    }
    // i915 / xe
    if (const auto mhz = internal::read_number_attribute<std::uint64_t>(card / "gt_max_freq_mhz")) {
      gpu.max_frequency = *mhz * FrequencyUnit::MHz;
    }
    result.push_back(std::move(gpu));
  }

  internal::gpu::enrich(result, query);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
