// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/disk.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "internal/block.h"
#include "internal/disk_vendor.h"
#include "internal/file.h"
#include "internal/pci.h"
#include "internal/procfs.h"
#include "internal/strings.h"
#include "internal/udev.h"
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
const fs::path udev_data = "/run/udev/data";

// Kernel name ("nvme0n1p2", "dm-0") of the block device with the given device number. nullopt for 0:x (filesystems
// without a block device, and btrfs, which reports an anonymous device number).
std::optional<std::string> kernel_name(std::uint32_t major, std::uint32_t minor) {
  if (major == 0) {
    return std::nullopt;
  }
  std::error_code ec;
  const auto name = fs::canonical(std::format("/sys/dev/block/{}:{}", major, minor), ec).filename().string();
  if (ec || name.empty() || !fs::exists(sys_class_block / name, ec)) {
    return std::nullopt;
  }
  return name;
}

// Kernel name of the block device a mount's source names, also through symlinks like /dev/mapper/* or
// /dev/disk/by-uuid/*. nullopt for sources that are no block device (tmpfs, proc, ...) and for "/dev/root".
std::optional<std::string> kernel_name(const std::string& source) {
  if (!source.starts_with('/')) {
    return std::nullopt;
  }
  std::error_code ec;
  const auto name = fs::canonical(source, ec).filename().string();
  if (ec || name.empty() || !fs::exists(sys_class_block / name, ec)) {
    return std::nullopt;
  }
  return name;
}

// The udev properties of a block device ("<major>:<minor>"); empty without udev (e.g. in containers).
std::map<std::string, std::string, std::less<>> udev_properties(std::string_view device_number) {
  return internal::read_file(udev_data / std::format("b{}", device_number))
      .transform(internal::udev::parse_properties)
      .value_or(std::map<std::string, std::string, std::less<>>{});
}

// ZFS pool name -> kernel names of its member devices (vdevs), from the udev database ("ID_FS_TYPE=zfs_member",
// "ID_FS_LABEL=<pool>"). Read on first use.
class ZfsPools {
 public:
  const std::vector<std::string>& members(std::string_view pool) {
    if (!_pools) {
      _pools.emplace();
      std::error_code ec;
      for (const auto& entry : fs::directory_iterator(udev_data, ec)) {
        const auto file = entry.path().filename().string();  // "b8:1"
        if (!file.starts_with('b')) {
          continue;
        }
        const auto properties = udev_properties(std::string_view(file).substr(1));
        const auto type = properties.find("ID_FS_TYPE");
        const auto label = properties.find("ID_FS_LABEL");
        if (type == properties.end() || type->second != "zfs_member" || label == properties.end()) {
          continue;
        }
        const auto colon = file.find(':');
        const auto major = internal::parse<std::uint32_t>(std::string_view(file).substr(1, colon - 1));
        const auto minor = internal::parse<std::uint32_t>(std::string_view(file).substr(colon + 1));
        if (colon == std::string::npos || !major || !minor) {
          continue;
        }
        if (auto name = kernel_name(*major, *minor)) {
          (*_pools)[label->second].push_back(std::move(*name));
        }
      }
    }
    static const std::vector<std::string> none;
    const auto it = _pools->find(pool);
    return it == _pools->end() ? none : it->second;
  }

 private:
  std::optional<std::map<std::string, std::vector<std::string>, std::less<>>> _pools;
};

struct ResolvedMount {
  MountPoint mount_point;
  std::set<std::string, std::less<>> disks;  // whole disks the filesystem is stored on
};

ResolvedMount resolve(const internal::procfs::MountInfo& mount, ZfsPools& zfs) {
  ResolvedMount resolved{
      .mount_point =
          {
              .path = mount.mount_point,
              .filesystem = internal::non_empty(mount.fs_type),
              .source = internal::non_empty(mount.source),
              .space_id = std::format("{}:{}", mount.major, mount.minor),
          },
      .disks = {},
  };
  if (mount.fs_type == "zfs") {
    // A dataset draws from its pool; ZFS encrypts per dataset, invisible here
    const auto pool = internal::procfs::zfs_pool(mount.source);
    resolved.mount_point.space_id = std::format("zfs:{}", pool);
    for (const auto& member : zfs.members(pool)) {
      resolved.disks.merge(internal::block::resolve_stack(sys_class_block, member).disks);
    }
    return resolved;
  }
  // The device number also resolves sources that are no device path ("/dev/root" without initramfs)
  const auto name = kernel_name(mount.major, mount.minor).or_else([&] { return kernel_name(mount.source); });
  if (!name) {
    return resolved;  // tmpfs, NFS, overlay, ...
  }
  auto stack = internal::block::resolve_stack(sys_class_block, *name);
  resolved.mount_point.volume = *name;
  resolved.mount_point.encrypted = stack.encrypted;
  if (!stack.encrypted && mount.fs_type == "bcachefs") {
    resolved.mount_point.encrypted = std::nullopt;  // encrypts itself, invisible in the block device stack
  }
  resolved.disks = std::move(stack.disks);
  return resolved;
}

result<std::vector<internal::procfs::MountInfo>> mounts() {
  return internal::read_file("/proc/self/mountinfo").transform(internal::procfs::parse_mountinfo);
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

  ZfsPools zfs;
  std::map<std::string, std::vector<MountPoint>, std::less<>> mounts_by_disk;
  for (const auto& mount : mounts().value_or(std::vector<internal::procfs::MountInfo>{})) {
    auto resolved = resolve(mount, zfs);
    for (const auto& disk_name : resolved.disks) {
      mounts_by_disk[disk_name].push_back(resolved.mount_point);
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
    // ATA drives, also behind USB-SATA bridges, whose SCSI vendor / model sysfs shows: "ATA" and the model truncated
    // to 16 characters, or the bridge (e.g. "ASMT" / "2115"). udev's ata_id read the drive's own identity.
    const auto properties = udev_properties(internal::read_attribute(block / "dev").value_or(""));
    if (const auto ata = properties.find("ID_ATA"); ata != properties.end() && ata->second == "1") {
      const auto property = [&](std::string_view key) -> std::optional<std::string> {
        const auto it = properties.find(key);
        return it == properties.end() ? std::nullopt : internal::udev::decode(it->second);
      };
      if (auto model = property("ID_MODEL_ENC").or_else([&] { return property("ID_MODEL"); })) {
        disk.model = std::move(model);
        disk.vendor = internal::disk_vendor_from_model(*disk.model);  // ATA has no vendor field
      }
      if (auto serial = property("ID_SERIAL_SHORT")) {
        disk.serial_number = std::move(serial);
      }
    } else if (disk.vendor == "ATA" && disk.model) {
      disk.vendor = internal::disk_vendor_from_model(*disk.model);
    }
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

result<MountPoint> mount_point_of(const fs::path& path) {
  std::error_code ec;
  const auto canonical = fs::canonical(path, ec);
  if (ec) {
    return std::unexpected(error{ec, path.string()});
  }
  const auto all = mounts();
  if (!all) {
    return std::unexpected(all.error());
  }
  const auto* mount = internal::procfs::containing_mount(*all, canonical.string());
  if (mount == nullptr) {
    return std::unexpected(error{errc::not_found, std::format("no mount contains {}", canonical.string())});
  }
  ZfsPools zfs;
  return resolve(*mount, zfs).mount_point;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
