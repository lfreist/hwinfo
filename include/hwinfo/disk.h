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

  const std::string& vendor() const;
  const std::string& model() const;
  const std::string& serialNumber() const;
  int64_t size_Bytes() const;
  int id() const;

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