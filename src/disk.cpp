// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <filesystem>
#include <fstream>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <sys/statvfs.h>
#endif

#include "hwinfo/disk.h"
#include "hwinfo/utils/stringutils.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
Disk::Disk(const std::string &vendor, const std::string &model, const std::string &serialNumber, int64_t size_Bytes) {
  _vendor = vendor;
  _model = model;
  _serialNumber = serialNumber;
  _size_Bytes = size_Bytes;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::vendor() const {
  return _vendor;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::model() const {
  return _model;
}

// _____________________________________________________________________________________________________________________
const std::string &Disk::serialNumber() const {
  return _serialNumber;
}

// _____________________________________________________________________________________________________________________
int64_t Disk::size_Bytes() const {
  return _size_Bytes;
}


// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
#if defined(unix) || defined(__unix) || defined(__unix__)
  const std::string base_path("/sys/class/block/");
  std::string vendor;
  std::string model;
  std::string serialNumber;
  for (const auto& entry: std::filesystem::directory_iterator(base_path)) {
    std::string path = entry.path().string() + "/device/";
    if (!std::filesystem::exists(std::filesystem::path(path))) {
      continue;
    }
    std::ifstream f(path + "vendor");
    if (f) {
      getline(f, vendor);
    }
    else { vendor = "<unknown>"; }
    f.close();
    f.open(path + "model");
    if (f) {
      getline(f, model);
    }
    else { vendor = "<unknown>"; }
    f.close();
    f.open(path + "serial");
    if (f) {
      getline(f, serialNumber);
    }
    else { serialNumber = "<unknown>"; }
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
#elif defined(__APPLE__)
  return disks;
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  return disks;
#endif
  return disks;
}

}  // namespace hwinfo
