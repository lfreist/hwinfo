// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

const unsigned short block_size = 512; // Linux always considers sectors to be 512 bytes long independently of the devices real block size.

class Disk {
  friend std::vector<Disk> getAllDisks();

 public:
  ~Disk() = default;

  const std::string& vendor() const;
  const std::string& model() const;
  const std::string& serialNumber() const;
  int64_t size_Bytes() const;
  int id() const;
  bool empty() const;

 private:
  Disk() {
    _vendor = "<unknown>";
    _model = "<unknown>";
    _serialNumber = "<unknown>";
  }
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  int64_t _size_Bytes{-1};
  int _id{-1};
};

std::vector<Disk> getAllDisks();
std::string getDiskVendor(const std::string& path);
std::string getDiskModel(const std::string& path);
std::string getDiskSerialNumber(const std::string& path);
int64_t getDiskSize_Bytes(const std::string& path);
bool isPartition(const std::string& path);

}  // namespace hwinfo
