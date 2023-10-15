// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

class Disk {
  friend std::vector<Disk> getAllDisks();

 public:
  ~Disk() = default;

  const std::string& vendor() const { return _vendor; }
  const std::string& model() const { return _model; }
  const std::string& serialNumber() const { return _serialNumber; }
  int64_t size_Bytes() const { return _size_Bytes; }
  int id() const { return _id; }

 private:
  Disk() = default;
  std::string _vendor;
  std::string _model;
  std::string _serialNumber;
  int64_t _size_Bytes{-1};
  int _id{-1};
};

std::vector<Disk> getAllDisks();

}  // namespace hwinfo