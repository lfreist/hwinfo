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

// A mounted filesystem: on a disk (or one of its partitions / volumes), or on none (tmpfs, NFS, overlay, ...).
struct MountPoint {
  std::filesystem::path path{};
  std::optional<std::string> filesystem{};  // as named by the OS, e.g. "ext4", "apfs", "NTFS", "tmpfs", "nfs4"
  // What is mounted, as the OS names it, e.g. "/dev/nvme0n1p2", "tank/home" (ZFS dataset), "server:/export", "tmpfs".
  // Not on Windows.
  std::optional<std::string> source{};
  // The block device: kernel name on Linux ("nvme0n1p2", "dm-0"), BSD name on macOS ("disk3s1"), volume GUID path on
  // Windows ("\\?\Volume{...}\"). Empty for mounts that are on no block device.
  std::optional<std::string> volume{};
  // Identity of the storage space the filesystem allocates from: mounts with equal space_id share their free space, so
  // sum free / total space over distinct ids only. Opaque, and only meaningful within one boot. It identifies
  // - Linux: the filesystem ("major:minor" of /proc/self/mountinfo), so bind mounts and btrfs subvolumes share it;
  //   ZFS datasets share the pool ("zfs:<pool>"),
  // - macOS: the APFS container ("disk3") for APFS volumes, the volume itself for other volumes, the fsid for
  //   filesystems on no device (NFS, SMB, devfs, ...),
  // - Windows: the volume (volume GUID path), the share ("\\server\share\") for network drives.
  std::optional<std::string> space_id{};
  std::optional<bool> encrypted{};

  friend bool operator==(const MountPoint&, const MountPoint&) = default;
};

// A physical (or virtual) block device.
struct Disk {
  std::uint32_t index = 0;
  std::optional<std::string> vendor{};
  std::optional<std::string> model{};
  std::optional<std::string> serial_number{};
  Bytes size{};
  DiskBus bus = DiskBus::unknown;
  std::optional<DataRate> link_speed{};  // negotiated link speed (currently USB only)
  std::vector<MountPoint> mount_points{};

  friend bool operator==(const Disk&, const Disk&) = default;
};

// All disks of the system.
// See hwinfo/monitoring.h for free space.
[[nodiscard]] HWINFO_API result<std::vector<Disk>> disks();

// The mounted filesystem that contains `path` (which must exist), also if it is on no disk (tmpfs, NFS, overlay, ...).
// Symbolic links are followed. If several filesystems are mounted on the same mount point, the topmost (visible) one.
[[nodiscard]] HWINFO_API result<MountPoint> mount_point_of(const std::filesystem::path& path);

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

inline std::string to_string(const MountPoint& mount_point) {
  return std::format("{} ({})", mount_point.path.string(), mount_point.filesystem.value_or("unknown"));
}

inline std::string to_string(const Disk& disk) {
  return std::format("{} ({}, {})", disk.model.value_or("unknown disk"), disk.size, to_string(disk.bus));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::DiskBus> : hwinfo::detail::to_string_formatter<hwinfo::DiskBus> {};

template <>
struct std::formatter<hwinfo::MountPoint> : hwinfo::detail::to_string_formatter<hwinfo::MountPoint> {};

template <>
struct std::formatter<hwinfo::Disk> : hwinfo::detail::to_string_formatter<hwinfo::Disk> {};
