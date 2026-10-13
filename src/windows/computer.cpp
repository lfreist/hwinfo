// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/computer.h>
#include <windows.h>

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include "internal/cim.h"
#include "internal/dmi.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

// Win32_ComputerSystem.PCSystemTypeEx (coarser than the SMBIOS chassis type)
ChassisType chassis(std::optional<std::uint16_t> pc_system_type) {
  switch (pc_system_type.value_or(0)) {
    case 1:  // desktop
    case 3:  // workstation
      return ChassisType::desktop;
    case 2:  // mobile
      return ChassisType::laptop;
    case 4:  // enterprise server
    case 5:  // SOHO server
    case 7:  // performance server
      return ChassisType::server;
    case 6:  // appliance PC
      return ChassisType::other;
    case 8:  // slate
      return ChassisType::tablet;
    default:
      return ChassisType::unknown;
  }
}

}  // namespace

result<Computer> computer() {
  const auto systems = internal::wmi::query(
      "Win32_ComputerSystem", {"Manufacturer", "Model", "SystemFamily", "SystemSKUNumber", "PCSystemTypeEx"});
  if (!systems) {
    return std::unexpected(systems.error());
  }
  if (systems->empty()) {
    return std::unexpected(error{errc::not_found, "WMI: Win32_ComputerSystem"});
  }
  const internal::wmi::Row& system = systems->front();
  const auto products = internal::wmi::query("Win32_ComputerSystemProduct", {"Version", "IdentifyingNumber"});
  const internal::wmi::Row empty;
  const internal::wmi::Row& product = products && !products->empty() ? products->front() : empty;
  const auto operating_systems = internal::wmi::query("Win32_OperatingSystem", {"LastBootUpTime"});
  // SMBIOS chassis type (the first enclosure's first entry), as on Linux
  const auto enclosures = internal::wmi::query("Win32_SystemEnclosure", {"ChassisTypes"});
  std::optional<std::uint8_t> chassis_code;
  if (enclosures && !enclosures->empty()) {
    if (const auto types = enclosures->front().numbers("ChassisTypes"); !types.empty() && types.front() > 0) {
      chassis_code = static_cast<std::uint8_t>(types.front() & 0x7f);
    }
  }
  ChassisType chassis_type = chassis_code ? internal::chassis_from_smbios(*chassis_code) : ChassisType::unknown;
  if (chassis_type == ChassisType::unknown) {
    chassis_type = chassis(system.number<std::uint16_t>("PCSystemTypeEx"));
  }
  const internal::wmi::Row& os = operating_systems && !operating_systems->empty() ? operating_systems->front() : empty;
  return Computer{
      .vendor = internal::dmi_string(system.string("Manufacturer")),
      .model = internal::dmi_string(system.string("Model")),
      .family = internal::dmi_string(system.string("SystemFamily")),
      .version = internal::dmi_string(product.string("Version")),
      .sku = internal::dmi_string(system.string("SystemSKUNumber")),
      .serial_number = internal::dmi_string(product.string("IdentifyingNumber")),
      .chassis = chassis_type,
      .chassis_code = chassis_code,
      // the boot time Windows keeps (now - uptime would be off by up to a second)
      .boot_time = os.string("LastBootUpTime").and_then(internal::cim::parse_datetime),
  };
}

// GetTickCount64 includes time spent asleep and hibernated.
result<std::chrono::seconds> uptime() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(GetTickCount64()));
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
