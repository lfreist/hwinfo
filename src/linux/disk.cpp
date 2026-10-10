// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/disk.h>

#include <algorithm>
#include <array>
#include <filesystem>
#include <set>
#include <string>
#include <vector>

#include "internal/file.h"
#include "internal/procfs.h"
#include "internal/strings.h"

namespace hwinfo {

namespace {

namespace fs = std::filesystem;

// Linux always reports sizes in 512 byte sectors, independent of the device's real block size.
constexpr std::uint64_t sector_size = 512;

bool is_virtual_device(std::string_view name) {
  constexpr std::array prefixes{"loop", "ram", "zram", "dm-", "md", "nbd"};
  return std::ranges::any_of(prefixes, [&](std::string_view prefix) { return name.starts_with(prefix); });
}

// Walks up the device hierarchy to the USB device and reads its negotiated speed (Mbit/s).
std::optional<DataRate> usb_link_speed(const fs::path& device) {
  std::error_code ec;
  for (fs::path current = fs::canonical(device, ec); !ec && current.has_relative_path();
       current = current.parent_path()) {
    if (const auto mbps = internal::read_number_attribute<double>(current / "speed")) {
      return DataRate{static_cast<std::uint64_t>(*mbps * 1e6)};  // "1.5" for USB 1.0 low speed
    }
  }
  return std::nullopt;
}

DiskBus disk_bus(const fs::path& block) {
  std::error_code ec;
  const std::string device = fs::canonical(block / "device", ec).string();
  if (ec) {
    return DiskBus::unknown;
  }
  if (device.contains("/usb")) {  // before ata/scsi: USB mass storage is attached via SCSI
    return DiskBus::usb;
  }
  if (device.contains("/nvme")) {
    return DiskBus::nvme;
  }
  if (device.contains("/ata")) {
    return DiskBus::sata;
  }
  if (device.contains("/mmc")) {
    return DiskBus::mmc;
  }
  if (device.contains("/virtio")) {
    return DiskBus::virtio;
  }
  if (device.contains("/host")) {
    return DiskBus::scsi;
  }
  return DiskBus::unknown;
}

}  // namespace

result<std::vector<Disk>> disks() {
  const fs::path sys_block = "/sys/block";
  std::error_code ec;
  fs::directory_iterator it(sys_block, ec);
  if (ec) {
    return std::unexpected(error{ec, sys_block.string()});
  }
  std::vector<fs::path> blocks;
  for (const auto& entry : it) {
    if (!is_virtual_device(entry.path().filename().string())) {
      blocks.push_back(entry.path());
    }
  }
  std::ranges::sort(blocks);

  const auto mounts = internal::read_file("/proc/self/mounts")
                          .transform(internal::procfs::parse_mounts)
                          .value_or(std::vector<internal::procfs::Mount>{});

  std::vector<Disk> result;
  for (const auto& block : blocks) {
    const std::string name = block.filename().string();
    Disk disk{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = internal::read_attribute(block / "device/vendor"),
        .model = internal::read_attribute(block / "device/model"),
        .serial_number = internal::read_attribute(block / "device/serial").or_else([&] {
          return internal::read_attribute(block / "serial");
        }),
        .size = {internal::read_number_attribute<std::uint64_t>(block / "size").value_or(0) * sector_size},
        .bus = disk_bus(block),
    };
    if (disk.bus == DiskBus::usb) {
      disk.link_speed = usb_link_speed(block / "device");
    }

    // the disk itself and its partitions (subdirectories containing a "partition" file)
    std::set<std::string, std::less<>> devices{"/dev/" + name};
    for (const auto& sub : fs::directory_iterator(block, ec)) {
      if (fs::exists(sub.path() / "partition", ec)) {
        devices.insert("/dev/" + sub.path().filename().string());
      }
    }
    for (const auto& mount : mounts) {
      if (devices.contains(mount.device)) {
        disk.mount_points.push_back({mount.mount_point, internal::non_empty(mount.fs_type)});
      }
    }
    result.push_back(std::move(disk));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
