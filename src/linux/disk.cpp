// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/disk.h>
#include <hwinfo/utils/filesystem.h>
#include <hwinfo/utils/stringutils.h>

#include <fstream>
#include <regex>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
bool isPartition(const std::string& path) {
  std::regex partitionRegex(R"((sd[a-z]|nvme\d+n\d+)p?\d+$)");
  return std::regex_search(path, partitionRegex);
}

// _____________________________________________________________________________________________________________________
std::string getDiskVendor(const std::string& path) {
  // nvme devices are in /sys/class/nvme/ and /sys/class/block/nvme*
  // but the vendor file is only in /sys/class/nvme/
  // so we need to check if the device is in /sys/class/block/nvme* and if so, we need to change the path
  // TODO: use utils::get_specs_by_file
  std::string vendor_path = path;
  std::string::size_type nvme_pos = path.find("nvme");
  if (nvme_pos != std::string::npos) {
    std::string nvme_name = path.substr(nvme_pos, 5);  // Exemple : "nvme0"
    vendor_path = path.substr(0, nvme_pos - 6) + "nvme/" + nvme_name;
  }

  std::ifstream f(vendor_path + "/device/vendor");
  if (f.is_open()) {
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
    if (!filesystem::exists(path)) continue;
    if (isPartition(path)) {  // Check if it's a partition
      continue;
    }
    Disk disk = Disk();
    disk._vendor = getDiskVendor(path);
    disk._model = getDiskModel(path);
    disk._serialNumber = getDiskSerialNumber(path);
    // Check before get size because size is always define in /sys/class/block/...
    if (disk._vendor == "<unknown>" && disk._model == "<unknown>" && disk._serialNumber == "<unknown>") {
      continue;
    }
    disk._size_Bytes = getDiskSize_Bytes(path);

    disks.push_back(std::move(disk));
  }
  return disks;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
