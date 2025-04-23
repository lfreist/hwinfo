// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

// Linux always considers sectors to be 512 bytes long independently of the devices real block size.
const unsigned short block_size = 512;

class HWINFO_API Disk {
  friend std::vector<Disk> getAllDisks();

 public:
  ~Disk() = default;

  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD const std::string& model() const;
  HWI_NODISCARD const std::string& serialNumber() const;
  HWI_NODISCARD int64_t size_Bytes() const;
  HWI_NODISCARD int64_t free_size_Bytes() const;
  HWI_NODISCARD const std::vector<std::string>& volumes() const;
  HWI_NODISCARD int id() const;

 private:
  Disk() = default;

  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  int64_t _size_Bytes{-1};
  int64_t _free_size_Bytes{-1};
  std::vector<std::string> _volumes;
  int _id{-1};
};

std::vector<Disk> getAllDisks();

}  // namespace hwinfo
