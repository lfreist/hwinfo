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
  std::string vendor;
  std::string model;
  std::string serialNumber;
  for (const auto& entry : filesystem::getDirectoryEntries(base_path)) {
    std::string path = base_path + "/" + entry + "/device/";
    if (!filesystem::exists(path)) {
      continue;
    }
    std::ifstream f(path + "vendor");
    if (f) {
      getline(f, vendor);
    } else {
      vendor = "<unknown>";
    }
    f.close();
    f.open(path + "model");
    if (f) {
      getline(f, model);
    } else {
      vendor = "<unknown>";
    }
    f.close();
    f.open(path + "serial");
    if (f) {
      getline(f, serialNumber);
    } else {
      serialNumber = "<unknown>";
    }
    f.close();
    strip(vendor);
    strip(model);
    strip(serialNumber);
    int64_t size = -1;
    /*
    struct statvfs buf {};
    std::string mount_path("/dev/");
    mount_path.append(split_get_index(entry.path().string(), "/", -1));
    if (statvfs(mount_path.c_str(), &buf)) {
      size = static_cast<int64_t>(buf.f_bsize * buf.f_bfree);
    }
    */
    disks.emplace_back(vendor, model, serialNumber, size);
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX