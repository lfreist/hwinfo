// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "internal/block.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <set>
#include <string>
#include <vector>

using hwinfo::internal::block::is_excluded_device;

TEST(Block, DisksAreKept) {
  for (const auto* name : {"sda", "nvme0n1", "mmcblk0", "mmcblk1", "vda", "xvda"}) {
    EXPECT_FALSE(is_excluded_device(name)) << name;
  }
}

TEST(Block, VirtualOpticalAndEmmcHardwarePartitionsAreExcluded) {
  for (const auto* name :
       {"loop0", "ram0", "zram0", "dm-0", "md127", "nbd0", "sr0", "mmcblk0boot0", "mmcblk0boot1", "mmcblk0rpmb"}) {
    EXPECT_TRUE(is_excluded_device(name)) << name;
  }
}

// Symbolic links need privileges on Windows.
#ifndef _WIN32

namespace {

namespace fs = std::filesystem;
using hwinfo::internal::block::is_encrypting_dm_uuid;
using hwinfo::internal::block::resolve_stack;

// A /sys/class/block in a temporary directory: entries are symlinks into a devices tree, as in sysfs.
class SysClassBlock : public ::testing::Test {
 protected:
  void SetUp() override {
    root = fs::temp_directory_path() /
           std::format("hwinfo_block_{}", ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(root);
    fs::create_directories(root / "class/block");
    // nvme0n1 with partitions p1 (unencrypted) and p2 (LUKS)
    add("nvme0n1", "devices/pci/nvme/nvme0n1");
    add("nvme0n1p1", "devices/pci/nvme/nvme0n1/nvme0n1p1", {}, true);
    add("nvme0n1p2", "devices/pci/nvme/nvme0n1/nvme0n1p2", {}, true);
    add("sda", "devices/pci/ata/sda");
  }
  void TearDown() override { fs::remove_all(root); }

  void add(const std::string& name, const std::string& device, const std::vector<std::string>& slaves = {},
           bool partition = false, const std::string& dm_uuid = {}) {
    fs::create_directories(root / device);
    fs::create_directory_symlink(fs::path("../..") / device, root / "class/block" / name);
    if (partition) {
      std::ofstream(root / device / "partition") << "1\n";
    }
    if (!slaves.empty()) {
      fs::create_directories(root / device / "slaves");
      for (const auto& slave : slaves) {
        const auto link = root / device / "slaves" / slave;
        fs::create_directory_symlink(fs::relative(root / "class/block" / slave, link.parent_path()), link);
      }
    }
    if (!dm_uuid.empty()) {
      fs::create_directories(root / device / "dm");
      std::ofstream(root / device / "dm/uuid") << dm_uuid << '\n';
    }
  }
  [[nodiscard]] fs::path sys_class_block() const { return root / "class/block"; }

  fs::path root;
};

using Disks = std::set<std::string, std::less<>>;

}  // namespace

TEST(Block, EncryptingDmUuids) {
  EXPECT_TRUE(is_encrypting_dm_uuid("CRYPT-LUKS2-0123456789abcdef-luks-0123"));
  EXPECT_TRUE(is_encrypting_dm_uuid("CRYPT-LUKS1-0123-root"));
  EXPECT_TRUE(is_encrypting_dm_uuid("CRYPT-PLAIN-swap"));
  EXPECT_TRUE(is_encrypting_dm_uuid("CRYPT-BITLK-0123-usb"));
  EXPECT_FALSE(is_encrypting_dm_uuid("CRYPT-VERITY-0123-root"));
  EXPECT_FALSE(is_encrypting_dm_uuid("CRYPT-INTEGRITY-0123-data"));
  EXPECT_FALSE(is_encrypting_dm_uuid("CRYPT-SUBDEV-0123-data_dif"));
  EXPECT_FALSE(is_encrypting_dm_uuid("LVM-abcdef"));
  EXPECT_FALSE(is_encrypting_dm_uuid(""));
}

TEST_F(SysClassBlock, PartitionsAndWholeDisks) {
  auto stack = resolve_stack(sys_class_block(), "nvme0n1p1");
  EXPECT_EQ(stack.disks, (Disks{"nvme0n1"}));
  EXPECT_FALSE(stack.encrypted);
  stack = resolve_stack(sys_class_block(), "sda");  // filesystem on the whole disk
  EXPECT_EQ(stack.disks, (Disks{"sda"}));
}

TEST_F(SysClassBlock, LvmOnLuks) {
  add("dm-0", "devices/virtual/block/dm-0", {"nvme0n1p2"}, false, "CRYPT-LUKS2-0123-luks-0123");
  add("dm-1", "devices/virtual/block/dm-1", {"dm-0"}, false, "LVM-abc");
  const auto stack = resolve_stack(sys_class_block(), "dm-1");
  EXPECT_EQ(stack.disks, (Disks{"nvme0n1"}));
  EXPECT_TRUE(stack.encrypted);
}

TEST_F(SysClassBlock, LvmSpanningAnUnencryptedDisk) {
  add("dm-0", "devices/virtual/block/dm-0", {"nvme0n1p2"}, false, "CRYPT-LUKS2-0123-luks-0123");
  add("dm-1", "devices/virtual/block/dm-1", {"dm-0", "sda"}, false, "LVM-abc");
  const auto stack = resolve_stack(sys_class_block(), "dm-1");
  EXPECT_EQ(stack.disks, (Disks{"nvme0n1", "sda"}));
  EXPECT_FALSE(stack.encrypted);
}

TEST_F(SysClassBlock, VerityIsNotEncryption) {
  add("dm-0", "devices/virtual/block/dm-0", {"nvme0n1p2"}, false, "CRYPT-VERITY-0123-root");
  const auto stack = resolve_stack(sys_class_block(), "dm-0");
  EXPECT_EQ(stack.disks, (Disks{"nvme0n1"}));
  EXPECT_FALSE(stack.encrypted);
}

TEST_F(SysClassBlock, RaidOverTwoDisks) {
  add("md0", "devices/virtual/block/md0", {"nvme0n1p1", "sda"});
  EXPECT_EQ(resolve_stack(sys_class_block(), "md0").disks, (Disks{"nvme0n1", "sda"}));
}

#endif  // _WIN32
