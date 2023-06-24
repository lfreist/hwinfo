// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>

namespace hwinfo {

class RAM {
 public:
  RAM();
  ~RAM() = default;

  const std::string& vendor() const;
  const std::string& name() const;
  const std::string& model() const;
  const std::string& serialNumber() const;
  int64_t total_Bytes() const;
  int64_t free_Bytes() const;
  int64_t available_Bytes() const;

 private:
  std::string _vendor{};
  std::string _name{};
  std::string _model{};
  std::string _serialNumber{};
  int64_t _total_Bytes = -1;
  int64_t _free_Bytes = -1;
  int64_t _available_Bytes = -1;
  int _frequency = -1;
};

}  // namespace hwinfo
