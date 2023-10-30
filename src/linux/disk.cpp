// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <fstream>

#include "hwinfo/disk.h"
#include "hwinfo/utils/filesystem.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string getDiskVendor(const std::string& path) {
  std::ifstream f(path + "/device/vendor");
  if (f) {
    std::string vendor;
    getline(f, vendor);
    utils::strip(vendor);
    f.close();
    return vendor;
  }
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getDiskModel(const std::string& path) {
  std::ifstream f(path + "/device/model");
  if (f) {
    std::string model;
    getline(f, model);
    utils::strip(model);
    f.close();
    return model;
  }
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getDiskSerialNumber(const std::string& path) {
  std::ifstream f(path + "/device/serial");
  if (f) {
    std::string serial;
    getline(f, serial);
    utils::strip(serial);
    f.close();
    return serial;
  }
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
int64_t getDiskSize_Bytes(const std::string& path) {
  std::ifstream f(path + "/size");
  if (f) {
    int64_t size;
    f >> size;
    f.close();
    return size * block_size;
  }
  return -1;
}

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
  const std::string base_path("/sys/class/block/");
  for (const auto& entry : filesystem::getDirectoryEntries(base_path)) {
    std::string path(base_path + entry);
    if (!filesystem::exists(path)) {
      continue;
    }
    Disk disk = Disk();
    disk._vendor = getDiskVendor(path);
    disk._model = getDiskModel(path);
    disk._serialNumber = getDiskSerialNumber(path);
    if (disk.empty()) // Check before get size because size is always define in /sys/class/block/...
      continue;
    disk._size_Bytes = getDiskSize_Bytes(path);

    disks.push_back(std::move(disk));
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
