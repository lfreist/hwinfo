// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "internal/apfs.h"
#include "internal/disk_vendor.h"
#include "internal/procfs.h"
#include "internal/udev.h"

namespace procfs = hwinfo::internal::procfs;
namespace udev = hwinfo::internal::udev;

namespace {

// A system booted without initramfs (root "/dev/root"), with tmpfs, NFS, an overmounted directory, btrfs subvolumes,
// a bind mount and ZFS datasets.
const std::vector<procfs::MountInfo>& mounts() {
  static const auto parsed = procfs::parse_mountinfo(
      "21 1 8:2 / / rw,relatime shared:1 - ext4 /dev/root rw\n"
      "22 21 0:20 / /tmp rw,nosuid shared:2 - tmpfs tmpfs rw\n"
      "23 21 0:44 / /mnt/nfs rw - nfs4 server:/export rw,vers=4.2\n"
      "24 21 0:45 / /mnt/data rw - btrfs /dev/sdb1 rw,subvol=/data\n"
      "25 21 0:45 /home /home rw - btrfs /dev/sdb1 rw,subvol=/home\n"
      "26 21 0:45 /data/www /srv/www rw - btrfs /dev/sdb1 rw,subvol=/data\n"
      "27 21 0:46 / /mnt/data rw - tmpfs tmpfs rw\n"
      "28 21 0:50 / /tank rw - zfs tank rw,xattr\n"
      "29 28 0:51 / /tank/home rw - zfs tank/home rw,xattr\n"
      "30 21 0:52 / /mnt/My\\040Disk rw - vfat /dev/sdc1 rw\n");
  return parsed;
}

}  // namespace

TEST(Mountinfo, DeviceNumberAndSource) {
  const auto& all = mounts();
  ASSERT_EQ(all.size(), 10u);
  EXPECT_EQ(all[0].major, 8u);
  EXPECT_EQ(all[0].minor, 2u);
  EXPECT_EQ(all[0].source, "/dev/root");
  EXPECT_EQ(all[1].major, 0u);
  EXPECT_EQ(all[1].minor, 20u);
  EXPECT_EQ(all[2].source, "server:/export");
  EXPECT_EQ(all[4].root, "/home");
  EXPECT_EQ(all[8].source, "tank/home");
  EXPECT_EQ(all[9].mount_point, "/mnt/My Disk");
}

TEST(Mountinfo, ContainingMount) {
  const auto& all = mounts();
  const auto find = [&](std::string_view path) {
    const auto* mount = procfs::containing_mount(all, path);
    return mount == nullptr ? std::string("none") : mount->mount_point + " " + mount->fs_type;
  };
  EXPECT_EQ(find("/"), "/ ext4");
  EXPECT_EQ(find("/etc/fstab"), "/ ext4");
  EXPECT_EQ(find("/tmp"), "/tmp tmpfs");
  EXPECT_EQ(find("/tmp/x/y"), "/tmp tmpfs");
  EXPECT_EQ(find("/tmpfoo"), "/ ext4");  // a prefix of the string, but not of the path
  EXPECT_EQ(find("/mnt/nfs/file"), "/mnt/nfs nfs4");
  EXPECT_EQ(find("/mnt/data/file"), "/mnt/data tmpfs");  // mounted over the btrfs subvolume
  EXPECT_EQ(find("/home/alice"), "/home btrfs");
  EXPECT_EQ(find("/tank/home/alice"), "/tank/home zfs");
  EXPECT_EQ(find("/tank/other"), "/tank zfs");
  EXPECT_EQ(find("/mnt/My Disk/a"), "/mnt/My Disk vfat");
  EXPECT_EQ(procfs::containing_mount({}, "/"), nullptr);
}

TEST(Mountinfo, ContainsPath) {
  EXPECT_TRUE(procfs::contains_path("/", "/"));
  EXPECT_TRUE(procfs::contains_path("/", "/a"));
  EXPECT_TRUE(procfs::contains_path("/a", "/a"));
  EXPECT_TRUE(procfs::contains_path("/a", "/a/b"));
  EXPECT_FALSE(procfs::contains_path("/a", "/ab"));
  EXPECT_FALSE(procfs::contains_path("/a/b", "/a"));
}

TEST(Mountinfo, ZfsPool) {
  EXPECT_EQ(procfs::zfs_pool("tank"), "tank");
  EXPECT_EQ(procfs::zfs_pool("tank/home/alice"), "tank");
  EXPECT_EQ(procfs::zfs_pool("tank@snapshot"), "tank");
  EXPECT_EQ(procfs::zfs_pool("rpool/ROOT/ubuntu_abc"), "rpool");
}

TEST(Udev, Properties) {
  const auto properties = udev::parse_properties(
      "S:disk/by-id/ata-Samsung_SSD_870_EVO_1TB_S6PTNX0R123456\n"
      "I:5345518\n"
      "E:ID_ATA=1\n"
      "E:ID_BUS=ata\n"
      "E:ID_MODEL=Samsung_SSD_870_EVO_1TB\n"
      "E:ID_MODEL_ENC=Samsung\\x20SSD\\x20870\\x20EVO\\x201TB\\x20\\x20\\x20\\x20\n"
      "E:ID_SERIAL_SHORT=S6PTNX0R123456\n"
      "E:ID_FS_LABEL=a=b\n"
      "E:=invalid\n"
      "G:systemd\n");
  EXPECT_EQ(properties.size(), 6u);
  EXPECT_EQ(properties.at("ID_ATA"), "1");
  EXPECT_EQ(properties.at("ID_SERIAL_SHORT"), "S6PTNX0R123456");
  EXPECT_EQ(properties.at("ID_FS_LABEL"), "a=b");
  EXPECT_EQ(udev::decode(properties.at("ID_MODEL_ENC")), "Samsung SSD 870 EVO 1TB");
}

TEST(Udev, Decode) {
  EXPECT_EQ(udev::decode("WDC\\x20WD40EFRX-68N32N0"), "WDC WD40EFRX-68N32N0");
  EXPECT_EQ(udev::decode("back\\\\slash"), "back\\\\slash");  // only \xHH is an escape
  EXPECT_EQ(udev::decode("bad\\xZZ"), "bad\\xZZ");
  EXPECT_EQ(udev::decode("end\\x2"), "end\\x2");
  EXPECT_EQ(udev::decode("\\x20\\x20"), std::nullopt);
}

TEST(DiskVendor, FromModel) {
  using hwinfo::internal::disk_vendor_from_model;
  EXPECT_EQ(disk_vendor_from_model("Samsung SSD 870 EVO 1TB"), "SAMSUNG");
  EXPECT_EQ(disk_vendor_from_model("WDC WD40EFRX-68N32N0"), "WDC");
  EXPECT_EQ(disk_vendor_from_model("KINGSTON SA400S37240G"), "KINGSTON");
  EXPECT_EQ(disk_vendor_from_model("ST4000DM004-2CV104"), std::nullopt);
}

TEST(Apfs, Container) {
  using hwinfo::internal::apfs_container;
  EXPECT_EQ(apfs_container("disk3s1"), "disk3");
  EXPECT_EQ(apfs_container("disk3s1s1"), "disk3");
  EXPECT_EQ(apfs_container("disk12s5"), "disk12");
  EXPECT_EQ(apfs_container("disk3"), "disk3");
  EXPECT_EQ(apfs_container("diskx"), "diskx");
  EXPECT_EQ(apfs_container("map auto_home"), "map auto_home");
}
