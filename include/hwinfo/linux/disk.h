// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <fstream>

#include "hwinfo/disk.h"
#include "hwinfo/utils/filesystem.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
  const std::string base_path("/sys/class/block/");
  for (const auto& entry : filesystem::getDirectoryEntries(base_path)) {
    Disk disk;
    std::string path(base_path + "/" + entry + "/device/");
    if (!filesystem::exists(path)) {
      continue;
    }
    std::ifstream f(path + "vendor");
    if (f) {
      getline(f, disk._vendor);
    } else {
      disk._vendor = "<unknown>";
    }
    f.close();
    f.open(path + "model");
    if (f) {
      getline(f, disk._model);
    } else {
      disk._model = "<unknown>";
    }
    f.close();
    f.open(path + "serial");
    if (f) {
      getline(f, disk._serialNumber);
    } else {
      disk._serialNumber = "<unknown>";
    }
    f.close();
    utils::strip(disk._vendor);
    utils::strip(disk._model);
    utils::strip(disk._serialNumber);
    disk._size_Bytes = -1;
    /*
    struct statvfs buf {};
    std::string mount_path("/dev/");
    mount_path.append(split_get_index(entry.path().string(), "/", -1));
    if (statvfs(mount_path.c_str(), &buf)) {
      size = static_cast<int64_t>(buf.f_bsize * buf.f_bfree);
    }
    */

    disks.push_back(std::move(disk));
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX