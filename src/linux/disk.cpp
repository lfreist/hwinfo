// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/disk.h>
#include <hwinfo/utils/filesystem.h>
#include <hwinfo/utils/stringutils.h>
#include <sys/statvfs.h>

#include <fstream>
#include <regex>
#include <vector>

namespace {

// _____________________________________________________________________________________________________________________
bool isPartition(const std::string& path) {
  static const std::regex partitionRegex(R"((sd[a-z]|nvme\d+n\d+)p?\d+$)");
  return std::regex_search(path, partitionRegex);
}

// _____________________________________________________________________________________________________________________
bool readFile(const std::string& path, std::string& outContent) {
  // Reads the file content and stores it in `outContent`. Returns true if successful, false otherwise.
  std::ifstream file(path);
  if (!file) return false;

  std::getline(file, outContent);
  hwinfo::utils::strip(outContent);
  return !outContent.empty();
}

// _____________________________________________________________________________________________________________________
std::string getMountPoint(const std::string& device) {
  std::ifstream mounts("/proc/mounts");
  std::string line, dev, mount_point;
  while (std::getline(mounts, line)) {
    std::istringstream iss(line);
    if (iss >> dev >> mount_point && dev == device) {
      return mount_point;
    }
  }
  return "/";
}

}  // anonymous namespace

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string getDiskVendor(const std::string& path) {
  // nvme devices are in /sys/class/nvme/ and /sys/class/block/nvme*
  // but the vendor file is only in /sys/class/nvme/
  // so we need to check if the device is in /sys/class/block/nvme* and if so, we need to change the path
  // TODO: use utils::get_specs_by_file
  std::string vendor_path = path;
  std::string::size_type nvme_pos = path.find("nvme");
  if (nvme_pos != std::string::npos) {
    std::string nvme_name = path.substr(nvme_pos, 5);  // e.g., "nvme0"
    vendor_path = path.substr(0, nvme_pos - 6) + "nvme/" + nvme_name;
  }

  std::string vendor;
  return readFile(vendor_path + "/device/vendor", vendor) ? vendor : "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getDiskModel(const std::string& path) {
  std::string model;
  return readFile(path + "/device/model", model) ? model : "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::string getDiskSerialNumber(const std::string& path) {
  std::string serial;
  return readFile(path + "/device/serial", serial) ? serial : "<unknown>";
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

// _____________________________________________________________________________________________________________________
int64_t getDiskFreeSize_Bytes(const std::string& path) {
  struct statvfs stat {};
  if (statvfs(path.c_str(), &stat) == 0)
    return static_cast<int64_t>(stat.f_bsize) * static_cast<int64_t>(stat.f_bavail);

  return -1;
}

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;
  const std::string base_path = "/sys/class/block/";

  for (const auto& entry : filesystem::getDirectoryEntries(base_path)) {
    std::string path = base_path + entry;
    if (!filesystem::exists(path) || isPartition(path)) continue;

    Disk disk;
    disk._vendor = getDiskVendor(path);
    disk._model = getDiskModel(path);
    disk._serialNumber = getDiskSerialNumber(path);

    // Check before get size because size is always define in /sys/class/block/...
    if (disk._vendor == "<unknown>" && disk._model == "<unknown>" && disk._serialNumber == "<unknown>") {
      continue;
    }

    disk._size_Bytes = getDiskSize_Bytes(path);

    auto mount_point = getMountPoint("/dev/" + entry);
    disk._free_size_Bytes = getDiskFreeSize_Bytes(mount_point);
    disk._volumes.push_back(std::move(mount_point));

    disks.push_back(std::move(disk));
  }

  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
