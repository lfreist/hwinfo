// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/mainboard.h>

#include "internal/dmi.h"
#include "internal/file.h"

namespace hwinfo {

result<Mainboard> mainboard() {
  const auto dmi = internal::dmi_directory();
  if (!dmi) {
    return std::unexpected(error{errc::not_supported, "no DMI information available"});
  }
  const auto read = [&](std::string_view name) { return internal::dmi_string(internal::read_attribute(*dmi / name)); };
  return Mainboard{
      .vendor = read("board_vendor"),
      .name = read("board_name"),
      .version = read("board_version"),
      .serial_number = read("board_serial"),
  };
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
