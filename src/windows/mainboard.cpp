// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/mainboard.h>

#include <vector>

#include "internal/dmi.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

result<Mainboard> mainboard() {
  return internal::wmi::query("Win32_BaseBoard", {"Manufacturer", "Product", "Version", "SerialNumber"})
      .and_then([](const std::vector<internal::wmi::Row>& rows) -> result<Mainboard> {
        if (rows.empty()) {
          return std::unexpected(error{errc::not_found, "WMI: Win32_BaseBoard"});
        }
        const auto& board = rows.front();
        return Mainboard{
            .vendor = internal::dmi_string(board.string("Manufacturer")),
            .name = internal::dmi_string(board.string("Product")),
            .version = internal::dmi_string(board.string("Version")),
            .serial_number = internal::dmi_string(board.string("SerialNumber")),
        };
      });
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
