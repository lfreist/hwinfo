// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/computer.h>
// clang-format off
#include <windows.h>
#include <powrprof.h>
#include <winternl.h>
// clang-format on

#include <chrono>
#include <cstdint>
#include <optional>
#include <vector>

#include "internal/dmi.h"
#include "internal/smbios.h"
#include "internal/uuid.h"
#include "internal/wmi_wrapper.h"

#ifdef _MSC_VER
#pragma comment(lib, "ntdll.lib")
#endif

namespace hwinfo {

namespace {

// Power platform role from the ACPI preferred PM profile, the source of Win32_ComputerSystem.PCSystemTypeEx (coarser
// than the SMBIOS chassis type)
ChassisType chassis(POWER_PLATFORM_ROLE role) {
  switch (role) {
    case PlatformRoleDesktop:
    case PlatformRoleWorkstation:
      return ChassisType::desktop;
    case PlatformRoleMobile:
      return ChassisType::laptop;
    case PlatformRoleEnterpriseServer:
    case PlatformRoleSOHOServer:
    case PlatformRolePerformanceServer:
      return ChassisType::server;
    case PlatformRoleAppliancePC:
      return ChassisType::other;
    case PlatformRoleSlate:
      return ChassisType::tablet;
    default:
      return ChassisType::unknown;
  }
}

// SYSTEM_TIMEOFDAY_INFORMATION (winternl.h declares it as opaque bytes only)
struct TimeOfDayInformation {
  LARGE_INTEGER boot_time;  // UTC, 100 ns since 1601-01-01
  LARGE_INTEGER current_time;
  LARGE_INTEGER time_zone_bias;
  ULONG time_zone_id;
  ULONG reserved;
  ULONGLONG boot_time_bias;
  ULONGLONG sleep_time_bias;
};
static_assert(sizeof(TimeOfDayInformation) == 48);

// The boot time Windows keeps, also the source of Win32_OperatingSystem.LastBootUpTime. Unlike now - uptime, it is
// not off by up to a second.
std::optional<std::chrono::sys_seconds> boot_time() {
  TimeOfDayInformation info{};
  if (NtQuerySystemInformation(SystemTimeOfDayInformation, &info, sizeof(info), nullptr) != 0 ||
      info.boot_time.QuadPart <= 0) {
    return std::nullopt;
  }
  using filetime_ticks = std::chrono::duration<std::int64_t, std::ratio<1, 10'000'000>>;
  constexpr std::chrono::seconds unix_epoch{11'644'473'600};  // 1601-01-01 to 1970-01-01
  return std::chrono::sys_seconds{std::chrono::floor<std::chrono::seconds>(filetime_ticks{info.boot_time.QuadPart}) -
                                  unix_epoch};
}

// Fallback if the raw SMBIOS table cannot be read. WMI decodes the same table.
result<Computer> computer_from_wmi() {
  const auto systems =
      internal::wmi::query("Win32_ComputerSystem", {"Manufacturer", "Model", "SystemFamily", "SystemSKUNumber"});
  if (!systems) {
    return std::unexpected(systems.error());
  }
  if (systems->empty()) {
    return std::unexpected(error{errc::not_found, "WMI: Win32_ComputerSystem"});
  }
  const internal::wmi::Row& system = systems->front();
  const auto products = internal::wmi::query("Win32_ComputerSystemProduct", {"Version", "IdentifyingNumber", "UUID"});
  const internal::wmi::Row empty;
  const internal::wmi::Row& product = products && !products->empty() ? products->front() : empty;
  // SMBIOS chassis type (the first enclosure's first entry), as on Linux
  const auto enclosures = internal::wmi::query("Win32_SystemEnclosure", {"ChassisTypes"});
  std::optional<std::uint8_t> chassis_code;
  if (enclosures && !enclosures->empty()) {
    if (const auto types = enclosures->front().numbers("ChassisTypes"); !types.empty() && types.front() > 0) {
      chassis_code = static_cast<std::uint8_t>(types.front() & 0x7f);
    }
  }
  return Computer{
      .vendor = internal::dmi_string(system.string("Manufacturer")),
      .model = internal::dmi_string(system.string("Model")),
      .family = internal::dmi_string(system.string("SystemFamily")),
      .version = internal::dmi_string(product.string("Version")),
      .sku = internal::dmi_string(system.string("SystemSKUNumber")),
      .serial_number = internal::dmi_string(product.string("IdentifyingNumber")),
      .uuid = internal::system_uuid(product.string("UUID").and_then(internal::parse_uuid)),
      .chassis = chassis_code ? internal::chassis_from_smbios(*chassis_code) : ChassisType::unknown,
      .chassis_code = chassis_code,
  };
}

}  // namespace

result<Computer> computer() {
  // GetSystemFirmwareTable needs no administrator rights; WMI only if it fails
  const auto& table = internal::system_table();
  auto computer = table ? internal::computer_identity(*table) : std::nullopt;
  if (!computer) {
    auto from_wmi = computer_from_wmi();
    if (!from_wmi) {
      return std::unexpected(from_wmi.error());
    }
    computer = *std::move(from_wmi);
  }
  if (computer->chassis == ChassisType::unknown) {
    computer->chassis = chassis(PowerDeterminePlatformRoleEx(POWER_PLATFORM_ROLE_V2));
  }
  computer->boot_time = boot_time();
  return *std::move(computer);
}

// GetTickCount64 includes time spent asleep and hibernated.
result<std::chrono::seconds> uptime() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::milliseconds(GetTickCount64()));
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
