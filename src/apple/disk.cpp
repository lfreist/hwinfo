// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <hwinfo/disk.h>
#include <sys/mount.h>
#include <sys/param.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "internal/apple_cf.h"
#include "internal/strings.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;
namespace fs = std::filesystem;

// BSD name ("disk3s1s1") -> mount points
using MountMap = std::unordered_map<std::string, std::vector<MountPoint>>;

MountMap mounted_devices() {
  MountMap mounts;
  const int count = getfsstat(nullptr, 0, MNT_NOWAIT);
  if (count <= 0) {
    return mounts;
  }
  std::vector<struct statfs> entries(static_cast<std::size_t>(count));
  const int filled = getfsstat(entries.data(), static_cast<int>(entries.size() * sizeof(struct statfs)), MNT_NOWAIT);
  entries.resize(static_cast<std::size_t>(std::clamp(filled, 0, count)));
  for (const auto& entry : entries) {
    std::string_view device = entry.f_mntfromname;  // e.g. "/dev/disk3s1s1"
    if (!device.starts_with("/dev/")) {
      continue;
    }
    device.remove_prefix(5);
    mounts[std::string(device)].push_back({entry.f_mntonname, internal::non_empty(entry.f_fstypename)});
  }
  return mounts;
}

// Mount points of the disk, its partitions and the volumes of APFS containers on it.
// These are all IOMedia descendants of the disk in the service plane.
std::vector<MountPoint> mount_points(io_registry_entry_t disk, const MountMap& mounts) {
  std::vector<MountPoint> result;
  const auto add = [&](io_registry_entry_t media) {
    const auto bsd_name = cf::string_property(media, CFSTR(kIOBSDNameKey));
    if (!bsd_name) {
      return;
    }
    if (const auto it = mounts.find(*bsd_name); it != mounts.end()) {
      result.insert(result.end(), it->second.begin(), it->second.end());
    }
  };
  add(disk);
  io_iterator_t raw = IO_OBJECT_NULL;
  if (IORegistryEntryCreateIterator(disk, kIOServicePlane, kIORegistryIterateRecursively, &raw) == KERN_SUCCESS) {
    const cf::io_ptr iterator(raw);
    for (const auto& child : cf::collect(iterator.get())) {
      if (IOObjectConformsTo(child.get(), kIOMediaClass)) {
        add(child.get());
      }
    }
  }
  std::ranges::sort(result, {}, &MountPoint::path);
  const auto duplicates = std::ranges::unique(result, {}, &MountPoint::path);
  result.erase(duplicates.begin(), duplicates.end());
  return result;
}

// Whole media that are part of another medium, e.g. APFS containers (synthesized disks) on a physical partition.
bool is_synthesized(io_registry_entry_t disk) {
  for (auto current = cf::parent(disk); current; current = cf::parent(current.get())) {
    if (IOObjectConformsTo(current.get(), kIOMediaClass)) {
      return true;
    }
  }
  return false;
}

// "Physical Interconnect" of the "Protocol Characteristics" (IOStorageProtocolCharacteristics.h).
DiskBus disk_bus(CFTypeRef protocol_characteristics) {
  const auto interconnect =
      cf::to_string(cf::dictionary_value(protocol_characteristics, CFSTR("Physical Interconnect")));
  if (!interconnect) {
    return DiskBus::unknown;
  }
  const std::string_view type = *interconnect;
  if (type == "USB") {
    return DiskBus::usb;
  }
  // Apple Silicon's internal SSD is attached via "Apple Fabric"; PCIe SSDs of Macs are NVMe (or AHCI before 2015)
  if (type == "PCI-Express" || type == "Apple Fabric" || type == "NVMe") {
    return DiskBus::nvme;
  }
  if (type == "SATA" || type == "ATA") {
    return DiskBus::sata;
  }
  if (type == "SAS" || type == "SCSI Parallel Interface" || type == "Fibre Channel Interface") {
    return DiskBus::scsi;
  }
  if (type == "Secure Digital" || type == "SD") {
    return DiskBus::mmc;
  }
  return DiskBus::unknown;  // e.g. "Virtual Interface" (disk images), "FireWire", "Thunderbolt"
}

}  // namespace

result<std::vector<Disk>> disks() {
  CFMutableDictionaryRef matching = IOServiceMatching(kIOMediaClass);
  if (matching != nullptr) {
    CFDictionarySetValue(matching, CFSTR(kIOMediaWholeKey), kCFBooleanTrue);
  }
  const auto media = cf::matching_services(matching, kIOMediaClass);
  if (!media) {
    return std::unexpected(media.error());
  }
  const MountMap mounts = mounted_devices();

  std::vector<Disk> result;
  for (const auto& medium : *media) {
    const io_registry_entry_t disk = medium.get();
    if (is_synthesized(disk)) {
      continue;
    }
    // published by the storage device (e.g. IONVMeBlockStorageDevice), an ancestor of the IOMedia
    const auto device = cf::search_property(disk, CFSTR("Device Characteristics"));
    const auto protocol = cf::search_property(disk, CFSTR("Protocol Characteristics"));

    auto model = cf::to_string(cf::dictionary_value(device.get(), CFSTR("Product Name")));
    if (!model) {
      // registry name, e.g. "APPLE SSD AP0512Q Media"
      model = cf::entry_name(disk).and_then([](std::string name) {
        if (name.ends_with(" Media")) {
          name.resize(name.size() - 6);
        }
        return internal::non_empty(name);
      });
    }
    auto vendor = cf::to_string(cf::dictionary_value(device.get(), CFSTR("Vendor Name")));
    if (!vendor && model && (model->contains("APPLE") || model->contains("Apple"))) {
      vendor = "Apple";
    }

    result.push_back(Disk{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = std::move(vendor),
        .model = std::move(model),
        .serial_number = cf::to_string(cf::dictionary_value(device.get(), CFSTR("Serial Number"))),
        .size = Bytes{cf::number_property<std::uint64_t>(disk, CFSTR(kIOMediaSizeKey)).value_or(0)},
        .bus = disk_bus(protocol.get()),
        .link_speed = std::nullopt,
        .mount_points = mount_points(disk, mounts),
    });
  }
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
