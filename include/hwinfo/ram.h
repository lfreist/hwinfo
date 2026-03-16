// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <cstdint>
#include <string>
#include <vector>
#include <limits>

namespace hwinfo {

class HWINFO_API Memory {
 public:
  static constexpr std::uint32_t invalid_id = std::numeric_limits<std::uint32_t>::max();

 public:
  struct Module {
    std::uint32_t id = invalid_id;
    std::string vendor;
    std::string name;
    std::string model;
    std::string serial_number;
    uint64_t total_Bytes = 0;
    uint64_t frequency_hz = 0;
  };

 public:
  Memory();
  ~Memory() = default;

  HWI_NODISCARD const std::vector<Memory::Module>& modules() const;
  HWI_NODISCARD uint64_t total_Bytes() const;
  HWI_NODISCARD uint64_t free_Bytes() const;
  HWI_NODISCARD uint64_t available_Bytes() const;

 private:
  std::vector<Memory::Module> _modules;
};

}  // namespace hwinfo
