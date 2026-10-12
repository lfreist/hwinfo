// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Linux block devices (/sys/block). Not part of the public API.

#pragma once

#include <algorithm>
#include <array>
#include <filesystem>
#include <functional>
#include <set>
#include <string>
#include <string_view>

#include "file.h"

namespace hwinfo::internal::block {

// Virtual block devices, optical drives (sr*) and the hardware partitions of eMMC (mmcblk0boot0, mmcblk0rpmb): not
// disks.
inline bool is_excluded_device(std::string_view name) {
  constexpr std::array prefixes{"loop", "ram", "zram", "dm-", "md", "nbd", "sr"};
  if (std::ranges::any_of(prefixes, [&](std::string_view prefix) { return name.starts_with(prefix); })) {
    return true;
  }
  return name.starts_with("mmcblk") && (name.contains("boot") || name.ends_with("rpmb"));
}

// Whether a device mapper uuid names a dm-crypt target that encrypts: "CRYPT-<type>-...", as set by cryptsetup. VERITY,
// INTEGRITY and SUBDEV targets do not encrypt.
inline bool is_encrypting_dm_uuid(std::string_view uuid) {
  if (!uuid.starts_with("CRYPT-")) {
    return false;
  }
  uuid.remove_prefix(6);
  constexpr std::array types{"LUKS1", "LUKS2", "PLAIN", "LOOPAES", "TCRYPT", "BITLK", "FVAULT2"};
  return std::ranges::contains(types, uuid.substr(0, uuid.find('-')));
}

struct Stack {
  std::set<std::string, std::less<>> disks;  // whole disks the device is stored on
  bool encrypted = false;  // every path down to the disks passes through an encrypting dm-crypt target
};

namespace detail {
inline bool resolve(const std::filesystem::path& sys_class_block, const std::string& name,
                    std::set<std::string, std::less<>>& disks, int depth) {
  if (depth > 16) {
    return false;
  }
  const auto block = sys_class_block / name;
  const bool encrypting = is_encrypting_dm_uuid(read_attribute(block / "dm" / "uuid").value_or(""));
  std::error_code ec;
  bool stacked = false;
  bool all_encrypted = true;
  for (const auto& slave : std::filesystem::directory_iterator(block / "slaves", ec)) {
    stacked = true;
    all_encrypted = resolve(sys_class_block, slave.path().filename().string(), disks, depth + 1) && all_encrypted;
  }
  if (stacked) {
    return encrypting || all_encrypted;
  }
  if (std::filesystem::exists(block / "partition", ec)) {
    const auto disk = std::filesystem::canonical(block, ec).parent_path().filename().string();
    if (!ec) {
      disks.insert(disk);
    }
  } else {
    disks.insert(name);
  }
  return encrypting;
}
}  // namespace detail

// Follows a block device (kernel name, e.g. "dm-1") down through device mapper / md stacks (LUKS, LVM, RAID) via their
// "slaves" to the disks it is stored on; partitions map to their disk. `sys_class_block` is /sys/class/block.
inline Stack resolve_stack(const std::filesystem::path& sys_class_block, const std::string& name) {
  Stack stack;
  stack.encrypted = detail::resolve(sys_class_block, name, stack.disks, 0);
  return stack;
}

}  // namespace hwinfo::internal::block
