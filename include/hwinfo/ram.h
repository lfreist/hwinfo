// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>

namespace hwinfo {

class RAM {
 public:
  RAM();
  ~RAM() = default;

  HWI_NODISCARD const std::string& vendor() const;
  HWI_NODISCARD const std::string& name() const;
  HWI_NODISCARD const std::string& model() const;
  HWI_NODISCARD const std::string& serialNumber() const;
  HWI_NODISCARD int64_t total_Bytes() const;
  HWI_NODISCARD int64_t frequency_Hz() const;
  int64_t free_Bytes();
  int64_t available_Bytes();

 private:
  std::string _vendor{"<unknown>"};
  std::string _name{"<unknown>"};
  std::string _model{"<unknown>"};
  std::string _serialNumber{"<unknown>"};
  int64_t _total_Bytes = -1;
  int64_t _frequency_Hz = -1;
};

}  // namespace hwinfo
