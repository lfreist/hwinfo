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
#include <filesystem>

namespace {

// Linux always considers sectors to be 512 bytes long independently of the devices real block size.
inline constexpr std::size_t block_size = 512;

// _____________________________________________________________________________________________________________________
std::string read_file(const std::string& path) {
  std::ifstream file(path);
  if (!file) return {};

  std::string content;
  std::getline(file, content);
  hwinfo::utils::strip(content);
  return content;
}

// _____________________________________________________________________________________________________________________
std::string getDiskVendor(const std::filesystem::path& path) {
  std::string result = read_file(path / "device/vendor");
  if (result.empty()) {
    return "<unknown>";
  }
  return result;
}

// _____________________________________________________________________________________________________________________
std::string getDiskModel(const std::filesystem::path& path) {
  std::string result = read_file(path / "device/model");
  if (result.empty()) {
    return "<unknown>";
  }
  return result;
}

// _____________________________________________________________________________________________________________________
std::string getDiskSerialNumber(const std::filesystem::path& path) {
  std::string result = read_file(path / "device/serial");
  if (result.empty()) {
    return "<unknown>";
  }
  return result;
}

// _____________________________________________________________________________________________________________________
uint64_t getDiskSize_Bytes(const std::filesystem::path& path) {
  std::ifstream f(path / "size");
  uint64_t size = 0;
  if (f && f >> size) {
    return size * block_size;
  }
  return 0;
}

hwinfo::Disk::Interface getDiskUsbVersion(const std::filesystem::path& disk_sys_path) {
  std::filesystem::path current = std::filesystem::canonical(disk_sys_path);

  while (current != "/" && current.has_parent_path()) {
    if (std::filesystem::exists(current / "speed")) {
      std::string speed_str = read_file(current / "speed");
      if (speed_str.empty()) {
        return hwinfo::Disk::Interface::USB;
      }

      try {
        int speed = std::stoi(speed_str);
        if (speed >= 80000) return hwinfo::Disk::Interface::USB4_80GBit;
        if (speed >= 40000) return hwinfo::Disk::Interface::USB4_40GBit;
        if (speed >= 20000) {
          std::string ver = read_file(current / "version");
          if (ver.find("4.") != std::string::npos) {
            return hwinfo::Disk::Interface::USB4_20GBit;
          }
          return hwinfo::Disk::Interface::USB3_20GBit;
        }
        if (speed >= 10000) return hwinfo::Disk::Interface::USB3_10GBit;
        if (speed >= 5000)  return hwinfo::Disk::Interface::USB3_5GBit;
        if (speed == 480)   return hwinfo::Disk::Interface::USB2;
        if (speed < 480) return hwinfo::Disk::Interface::USB1;
      } catch (...) {
        return hwinfo::Disk::Interface::USB;
      }
    }
    current = current.parent_path();
  }
  return hwinfo::Disk::Interface::USB;
}

// _____________________________________________________________________________________________________________________
hwinfo::Disk::Interface getDiskInterface(const std::filesystem::path& path) {
  std::string subsystem = std::filesystem::canonical(path / "device").string();
  if (subsystem.find("nvme") != std::string::npos) {
    return hwinfo::Disk::Interface::NVME;
  } else if (subsystem.find("usb") != std::string::npos) {
    return getDiskUsbVersion(path);
  } else if (subsystem.find("ata") != std::string::npos) {
    return hwinfo::Disk::Interface::SATA;
  } else if (subsystem.find("scsi") != std::string::npos) {
    return hwinfo::Disk::Interface::SCSI;
  }
  return hwinfo::Disk::Interface::UNKNOWN;
}

// _____________________________________________________________________________________________________________________
int64_t getDiskFreeSize_Bytes(const std::string& path) {
  struct statvfs stat {};
  if (statvfs(path.c_str(), &stat) == 0)
    return static_cast<int64_t>(stat.f_bsize) * static_cast<int64_t>(stat.f_bavail);

  return -1;
}

}  // anonymous namespace

namespace hwinfo {

// =====================================================================================================================
std::uint64_t Disk::free_size_bytes() const { return 0; }

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::vector<Disk> getAllDisks() {
  std::vector<Disk> disks;

  std::uint32_t id = 0;
  for (const auto& entry : std::filesystem::directory_iterator("/sys/class/block")) {
    std::string name = entry.path().filename().string();
    if (std::filesystem::exists(entry.path() / "partition") || name.find("loop") == 0 || name.find("ram") == 0) {
      // skip partitions, loop devices and virtual devices
      continue;
    }
    Disk disk;
    disk._id = id++;
    disk._model = getDiskModel(entry.path());
    disk._vendor = getDiskVendor(entry.path());
    disk._serialNumber = getDiskSerialNumber(entry.path());
    disk._size_Bytes = getDiskSize_Bytes(entry.path());
    disk._interface = getDiskInterface(entry.path());
    for (const auto& sub_entry : std::filesystem::directory_iterator(entry.path())) {
      if (std::filesystem::exists(sub_entry.path() / "partition")) {
        disk._mount_points.emplace_back("/dev/" + sub_entry.path().filename().string());
      }
    }

    disks.emplace_back(std::move(disk));
  }

  return disks;
}

// =====================================================================================================================
// _____________________________________________________________________________________________________________________
std::ostream& operator<<(std::ostream& os, const Disk::Interface& interface) {
  switch (interface) {
    case Disk::Interface::NVME:
      os << "Disk::Interface::NVME";
      break;
    case hwinfo::Disk::Interface::USB:
      os << "Disk::Interface::USB";
      break;
    case Disk::Interface::USB1:
      os << "Disk::Interface::USB1";
      break;
    case Disk::Interface::USB2:
      os << "Disk::Interface::USB2";
      break;
    case Disk::Interface::USB3_5GBit:
      os << "Disk::Interface::USB3_5GBit";
      break;
    case Disk::Interface::USB3_10GBit:
      os << "Disk::Interface::USB3_10GBit";
      break;
    case Disk::Interface::USB3_20GBit:
      os << "Disk::Interface::USB3_20GBit";
      break;
    case Disk::Interface::USB4_40GBit:
      os << "Disk::Interface::USB4_40GBit";
      break;
    case Disk::Interface::USB4_80GBit:
      os << "Disk::Interface::USB3_80GBit";
      break;
    case Disk::Interface::SATA:
      os << "Disk::Interface::SATA";
      break;
    case Disk::Interface::SCSI:
      os << "Disk::Interface::SCSI";
      break;
    case Disk::Interface::UNKNOWN:
      os << "Disk::Interface::UNKNOWN";
      break;
  }
  return os;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
