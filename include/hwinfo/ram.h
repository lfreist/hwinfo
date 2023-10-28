// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

class RAM {
  friend std::vector<RAM> getAllRAM();

 public:
  RAM();
  ~RAM() = default;

  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD const std::string& name() const;
  HWI_NODISCARD const std::string& model() const;
  HWI_NODISCARD const std::string& serialNumber() const;
  HWI_NODISCARD int64_t total_Bytes() const;
  HWI_NODISCARD int64_t frequency_Hz() const;
  HWI_NODISCARD int id() const;
  int64_t free_Bytes();
  int64_t available_Bytes();

 private:
  explicit RAM(int id);

  std::string _vendor{"<unknown>"};
  std::string _name{"<unknown>"};
  std::string _model{"<unknown>"};
  std::string _serialNumber{"<unknown>"};
  int64_t _total_Bytes = -1;
  int64_t _frequency_Hz = -1;
  int _id;
};

std::vector<RAM> getAllRAM();

}  // namespace hwinfo
