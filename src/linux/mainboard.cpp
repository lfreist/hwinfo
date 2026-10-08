// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/mainboard.h>

#include <array>
#include <filesystem>

#include "internal/file.h"

namespace hwinfo {

result<Mainboard> mainboard() {
  const std::array candidates{std::filesystem::path("/sys/devices/virtual/dmi/id"),
                              std::filesystem::path("/sys/class/dmi/id")};
  for (const auto& dmi : candidates) {
    if (!std::filesystem::exists(dmi)) {
      continue;
    }
    return Mainboard{
        .vendor = internal::read_attribute(dmi / "board_vendor"),
        .name = internal::read_attribute(dmi / "board_name"),
        .version = internal::read_attribute(dmi / "board_version"),
        .serial_number = internal::read_attribute(dmi / "board_serial"),
    };
  }
  return std::unexpected(error{errc::not_supported, "no DMI information available"});
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
