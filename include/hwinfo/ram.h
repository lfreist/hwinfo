// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hwinfo {

class Memory {
 public:
  struct Module {
    int id;
    std::string vendor;
    std::string name;
    std::string model;
    std::string serial_number;
    int64_t total_Bytes;
    int64_t frequency_Hz;
  };

 public:
  Memory();
  ~Memory() = default;

  HWI_NODISCARD const std::vector<Memory::Module>& modules() const;
  HWI_NODISCARD int64_t total_Bytes() const;
  HWI_NODISCARD int64_t free_Bytes() const;
  HWI_NODISCARD int64_t available_Bytes() const;

 private:
  std::vector<Memory::Module> _modules;
};

}  // namespace hwinfo
