// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>
#include <hwinfo/units.h>

#include <array>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo {

struct PcieLink {
  std::uint32_t generation = 0;
  std::uint32_t width = 0;

  friend bool operator==(const PcieLink&, const PcieLink&) = default;
};

struct PciDevice {
  std::uint16_t vendor_id = 0;
  std::uint16_t device_id = 0;
  // Bus address "domain:bus:device.function", e.g. "0000:01:00.0" (as lspci / nvidia-smi / sysfs print it).
  // Identifies the device stably across runs and reboots.
  std::optional<std::string> address{};
  std::optional<PcieLink> max_link{};  // fastest link supported by both the device and its slot (PCIe only)

  friend bool operator==(const PciDevice&, const PciDevice&) = default;
};

struct GpuUuid {
  std::array<std::uint8_t, 16> bytes{};

  friend bool operator==(const GpuUuid&, const GpuUuid&) = default;
};

enum class GpuType {
  unknown,
  integrated,   // part of the CPU package or chipset (Intel UHD / Iris Xe, AMD APUs, Apple Silicon, ARM SoCs)
  discrete,     // a separate chip with its own memory (graphics cards, laptop dGPUs)
  virtualized,  // emulated or paravirtualized adapter of a virtual machine (virtio-gpu, VMware SVGA, Hyper-V, ...)
};

// A compute API through which the GPU can be programmed, e.g. {"CUDA", "12.8"}, {"OpenCL", "3.0 NEO"},
// {"Level Zero", "1.13"}. Listed when the API's runtime library is installed and reports the device.
struct GpuApi {
  std::string name{};
  std::string version{};

  friend bool operator==(const GpuApi&, const GpuApi&) = default;
};

/**
 * A GPU.
 *
 * The device list and the basic fields come from the operating system. Everything else is filled in from vendor and
 * compute libraries found at runtime (NVML and CUDA for NVIDIA, Level Zero for Intel, OpenCL for all vendors, amdgpu /
 * KFD sysfs for AMD on Linux), preferring the vendor's own library. Fields stay empty when no source provides them.
 */
struct Gpu {
  std::uint32_t index = 0;  // position in the list returned by gpus(); enumeration order may change between boots
  std::string vendor{};
  std::string name{};
  GpuType type = GpuType::unknown;
  std::optional<bool> unified_memory{};
  std::optional<GpuUuid> uuid{};
  std::optional<std::uint64_t> luid{};  // Windows adapter LUID (as used by DXGI / D3D12); changes on reboot

  std::optional<std::string> architecture{};
  std::optional<std::string> compute_capability{};
  std::optional<std::uint32_t> compute_units{};
  std::optional<std::uint32_t> cores{};

  std::optional<std::string> driver{};
  std::optional<std::string> driver_version{};
  std::optional<std::string> vbios_version{};
  std::optional<std::string> serial{};

  std::optional<Bytes> dedicated_memory{};
  std::optional<Bytes> shared_memory{};
  std::optional<std::string> memory_type{};
  std::optional<std::uint32_t> memory_bus_width{};  // bits
  std::optional<Bytes> l2_cache{};

  std::optional<Hertz> max_frequency{};
  std::optional<Hertz> max_memory_frequency{};
  std::optional<Power> power_limit{};

  std::vector<GpuApi> compute_apis{};
  std::optional<PciDevice> pci{};  // absent for GPUs that are not PCI devices (Apple Silicon, most ARM SoCs)

  friend bool operator==(const Gpu&, const Gpu&) = default;
};

// Which runtime-loaded libraries gpus() may use in addition to the operating system. All are used by default; each one
// is loaded once per process and silently skipped when it is not installed. The first use of CUDA or OpenCL can take
// a few hundred milliseconds, and the vendor libraries wake up a discrete GPU that is powered down.
struct GpuQuery {
  bool nvml = true;        // NVIDIA Management Library (ships with the NVIDIA driver)
  bool cuda = true;        // CUDA driver API (ships with the NVIDIA driver)
  bool level_zero = true;  // oneAPI Level Zero loader + Intel compute runtime
  bool opencl = true;      // OpenCL ICD loader + any vendor's OpenCL driver

  // Operating system information only: fast, never wakes up sleeping GPUs.
  static constexpr GpuQuery os_only() noexcept {
    return {.nvml = false, .cuda = false, .level_zero = false, .opencl = false};
  }
};

// All GPUs of the system.
[[nodiscard]] HWINFO_API result<std::vector<Gpu>> gpus(const GpuQuery& query = {});

constexpr std::string_view to_string(GpuType type) noexcept {
  switch (type) {
    case GpuType::integrated:
      return "integrated";
    case GpuType::discrete:
      return "discrete";
    case GpuType::virtualized:
      return "virtualized";
    case GpuType::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const GpuUuid& uuid) {
  std::string out;
  for (std::size_t i = 0; i < uuid.bytes.size(); ++i) {
    if (i == 4 || i == 6 || i == 8 || i == 10) {
      out += '-';
    }
    out += std::format("{:02x}", uuid.bytes[i]);
  }
  return out;
}

inline std::string to_string(const PcieLink& link) { return std::format("PCIe {}.0 x{}", link.generation, link.width); }

inline std::string to_string(const Gpu& gpu) { return std::format("{} {}", gpu.vendor, gpu.name); }

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::GpuType> : hwinfo::detail::to_string_formatter<hwinfo::GpuType> {};

template <>
struct std::formatter<hwinfo::GpuUuid> : hwinfo::detail::to_string_formatter<hwinfo::GpuUuid> {};

template <>
struct std::formatter<hwinfo::PcieLink> : hwinfo::detail::to_string_formatter<hwinfo::PcieLink> {};

template <>
struct std::formatter<hwinfo::Gpu> : hwinfo::detail::to_string_formatter<hwinfo::Gpu> {};
