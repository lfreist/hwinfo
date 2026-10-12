// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/disk.h>

#include <algorithm>
#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "internal/block.h"
#include "internal/file.h"
#include "internal/pci.h"
#include "internal/procfs.h"
#include "internal/strings.h"
#include "pci_vendors.h"

namespace hwinfo {

namespace {

namespace fs = std::filesystem;

// Linux always reports sizes in 512 byte sectors, independent of the device's real block size.
constexpr std::uint64_t sector_size = 512;

std::string_view pci_vendor_database() { return {reinterpret_cast<const char*>(pci_vendors), pci_vendors_size}; }

// NVMe namespaces have no "vendor" attribute: name the PCI subsystem vendor of the controller (the brand, the PCI
// vendor is the maker of the controller chip).
std::optional<std::string> pci_vendor(const fs::path& block) {
  const fs::path pci = block / "device" / "device";
  std::error_code ec;
  const auto subsystem = fs::read_symlink(pci / "subsystem", ec);
  if (ec || subsystem.filename() != "pci") {
    return std::nullopt;
  }
  for (const auto* attribute : {"subsystem_vendor", "vendor"}) {
    if (const auto id = internal::read_number_attribute<std::uint16_t>(pci / attribute, 16); id && *id != 0) {
      return internal::lookup_pci(pci_vendor_database(), *id, 0).vendor;
    }
  }
  return std::nullopt;
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

const fs::path sys_class_block = "/sys/class/block";

// Kernel name ("nvme0n1p2", "dm-0") of the block device a mount's source names, also through symlinks like
// /dev/mapper/* or /dev/disk/by-uuid/*. nullopt for sources that are no block device (tmpfs, proc, ...).
std::optional<std::string> kernel_name(const std::string& device) {
  if (!device.starts_with('/')) {
    return std::nullopt;
  }
  std::error_code ec;
  const auto name = fs::canonical(device, ec).filename().string();
  if (ec || name.empty() || !fs::exists(sys_class_block / name, ec)) {
    return std::nullopt;
  }
  return name;
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
    if (!internal::block::is_excluded_device(entry.path().filename().string())) {
      blocks.push_back(entry.path());
    }
  }
  std::ranges::sort(blocks);

  const auto mounts = internal::read_file("/proc/self/mounts")
                          .transform(internal::procfs::parse_mounts)
                          .value_or(std::vector<internal::procfs::Mount>{});

  std::map<std::string, std::vector<MountPoint>, std::less<>> mounts_by_disk;
  for (const auto& mount : mounts) {
    const auto name = kernel_name(mount.device);
    if (!name) {
      continue;
    }
    const auto stack = internal::block::resolve_stack(sys_class_block, *name);
    MountPoint mount_point{
        .path = mount.mount_point,
        .filesystem = internal::non_empty(mount.fs_type),
        .volume = *name,
        .encrypted = stack.encrypted,
    };
    if (!stack.encrypted && mount.fs_type == "bcachefs") {
      mount_point.encrypted = std::nullopt;  // encrypts itself, invisible in the block device stack
    }
    for (const auto& disk_name : stack.disks) {
      mounts_by_disk[disk_name].push_back(mount_point);
    }
  }

  std::vector<Disk> result;
  for (const auto& block : blocks) {
    const std::string name = block.filename().string();
    const auto sectors = internal::read_number_attribute<std::uint64_t>(block / "size").value_or(0);
    if (sectors == 0) {
      continue;  // no medium, e.g. an empty card reader
    }
    Disk disk{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = internal::read_attribute(block / "device/vendor").or_else([&] { return pci_vendor(block); }),
        .model = internal::read_attribute(block / "device/model"),
        .serial_number = internal::read_attribute(block / "device/serial").or_else([&] {
          return internal::read_attribute(block / "serial");
        }),
        .size = {sectors * sector_size},
        .bus = disk_bus(block),
    };
    if (disk.bus == DiskBus::usb) {
      disk.link_speed = usb_link_speed(block / "device");
    }
    if (const auto it = mounts_by_disk.find(name); it != mounts_by_disk.end()) {
      disk.mount_points = std::move(it->second);
    }
    result.push_back(std::move(disk));
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
