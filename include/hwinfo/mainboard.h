// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include "platform.h"

#include <string>
#include <vector>

namespace hwinfo {

class MainBoard {
  friend std::string get_dmi_by_name(const std::string& name);

 public:
  MainBoard();
  ~MainBoard() = default;

  const std::string& vendor() const { return _vendor; }
  const std::string& name() const { return _name; }
  const std::string& version() const { return _version; }
  const std::string& serialNumber() const { return _serialNumber; }

 private:
  std::string _vendor;
  std::string _name;
  std::string _version;
  std::string _serialNumber;
};

}  // namespace hwinfo

#if defined(HWINFO_APPLE)
#include "apple/mainboard.h"
#elif defined(HWINFO_UNIX)
#include "linux/mainboard.h"
#elif defined(HWINFO_WINDOWS)
#include "windows/mainboard.h"
#endif
