// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/mainboard.h>
#include <windows.h>

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/dmi.h"
#include "internal/smbios.h"
#include "internal/strings.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

// Date of a CIM datetime ("20240514000000.000000+000"); WMI's BIOS release date carries no time.
std::optional<std::chrono::year_month_day> cim_date(const std::string& s) {
  if (s.size() < 8) {
    return std::nullopt;
  }
  const auto year = internal::parse<int>(std::string_view(s).substr(0, 4));
  const auto month = internal::parse<unsigned>(std::string_view(s).substr(4, 2));
  const auto day = internal::parse<unsigned>(std::string_view(s).substr(6, 2));
  if (!year || !month || !day) {
    return std::nullopt;
  }
  const std::chrono::year_month_day date{std::chrono::year(*year), std::chrono::month(*month), std::chrono::day(*day)};
  return date.ok() ? std::optional(date) : std::nullopt;
}

// Fallback if the raw SMBIOS table cannot be read. WMI decodes the same table.
result<Mainboard> mainboard_from_wmi() {
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
  if (const auto bios = internal::wmi::query("Win32_BIOS", {"Manufacturer", "ReleaseDate", "SMBIOSBIOSVersion"});
      bios && !bios->empty()) {
    mainboard.firmware_vendor = internal::dmi_string(bios->front().string("Manufacturer"));
    mainboard.firmware_version = internal::dmi_string(bios->front().string("SMBIOSBIOSVersion"));
    mainboard.firmware_date = bios->front().string("ReleaseDate").and_then(cim_date);
  }
  return mainboard;
}

}  // namespace

result<Mainboard> mainboard() {
  if (const auto table = smbios::read_system_table()) {
    if (auto board = internal::mainboard_info(*table)) {
      return *std::move(board);
    }
    return std::unexpected(error{errc::not_found, "SMBIOS: no baseboard or BIOS information"});
  }
  return mainboard_from_wmi();
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
