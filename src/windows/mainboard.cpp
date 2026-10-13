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
  const auto boards = internal::wmi::query("Win32_BaseBoard", {"Manufacturer", "Product", "Version", "SerialNumber"});
  if (!boards) {
    return std::unexpected(boards.error());
  }
  if (boards->empty()) {
    return std::unexpected(error{errc::not_found, "WMI: Win32_BaseBoard"});
  }
  const auto& board = boards->front();
  Mainboard mainboard{
      .vendor = internal::dmi_string(board.string("Manufacturer")),
      .name = internal::dmi_string(board.string("Product")),
      .version = internal::dmi_string(board.string("Version")),
      .serial_number = internal::dmi_string(board.string("SerialNumber")),
  };
  if (const auto bios = internal::wmi::query("Win32_BIOS", {"Manufacturer", "SMBIOSBIOSVersion"});
      bios && !bios->empty()) {
    mainboard.firmware_vendor = internal::dmi_string(bios->front().string("Manufacturer"));
    mainboard.firmware_version = internal::dmi_string(bios->front().string("SMBIOSBIOSVersion"));
  }
  return mainboard;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
