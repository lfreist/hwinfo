#pragma once

#include <hwinfo/platform.h>

#include <string>

namespace hwinfo {

class BIOS {
 public:
  BIOS();
  ~BIOS() = default;

  HWI_NODISCARD std::string name() const;
  HWI_NODISCARD std::string serialNumber() const;

 private:
  std::string _name;
  std::string _serialNumber;
  int x, y;
};

}  // namespace hwinfo
