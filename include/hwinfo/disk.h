// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/detail/formatter.h>
#include <hwinfo/error.h>
#include <hwinfo/platform.h>
#include <hwinfo/units.h>

#include <cstdint>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo {

enum class DiskBus { unknown, nvme, sata, scsi, usb, mmc, virtio };

// A physical (or virtual) block device.
struct Disk {
  std::uint32_t index = 0;
  std::optional<std::string> vendor{};
  std::optional<std::string> model{};
  std::optional<std::string> serial_number{};
  Bytes size{};
  DiskBus bus = DiskBus::unknown;
  std::optional<double> link_speed_gbps{};  // negotiated link speed (currently USB only)
  std::vector<std::filesystem::path> mount_points{};

  friend bool operator==(const Disk&, const Disk&) = default;
};

// All disks of the system.
// See hwinfo/monitoring.h for free space.
[[nodiscard]] HWINFO_API result<std::vector<Disk>> disks();

constexpr std::string_view to_string(DiskBus bus) noexcept {
  switch (bus) {
    case DiskBus::nvme:
      return "NVMe";
    case DiskBus::sata:
      return "SATA";
    case DiskBus::scsi:
      return "SCSI";
    case DiskBus::usb:
      return "USB";
    case DiskBus::mmc:
      return "MMC";
    case DiskBus::virtio:
      return "VirtIO";
    case DiskBus::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const Disk& disk) {
  return std::format("{} ({}, {})", disk.model.value_or("unknown disk"), disk.size, to_string(disk.bus));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::DiskBus> : hwinfo::detail::to_string_formatter<hwinfo::DiskBus> {};

template <>
struct std::formatter<hwinfo::Disk> : hwinfo::detail::to_string_formatter<hwinfo::Disk> {};
