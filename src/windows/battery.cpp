// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/battery.h>
#include <hwinfo/monitoring.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

using internal::wmi::Row;

// Win32_Battery.Chemistry, named like Linux' power_supply "technology"
std::optional<std::string> technology(std::optional<std::uint16_t> chemistry) {
  switch (chemistry.value_or(0)) {
    case 3:
      return "Lead Acid";
    case 4:
      return "NiCd";
    case 5:
      return "NiMH";
    case 6:
      return "Li-ion";
    case 7:
      return "Zinc Air";
    case 8:
      return "Li-poly";
    default:
      return std::nullopt;  // 1: other, 2: unknown
  }
}

std::optional<double> watt_hours(std::optional<std::uint32_t> mwh) {
  return mwh.value_or(0) > 0 ? std::optional(*mwh / 1000.0) : std::nullopt;
}

// Rows of a ROOT\WMI battery class if they (presumably) correspond one-to-one to the Win32_Battery rows.
// Those classes contain the data Win32_Battery often lacks, but may be inaccessible without elevated privileges.
std::vector<Row> acpi_rows(std::string_view wmi_class, std::initializer_list<std::string_view> properties,
                           std::size_t expected) {
  auto rows = internal::wmi::query(wmi_class, properties, {}, L"ROOT\\WMI");
  return rows && rows->size() == expected ? std::move(*rows) : std::vector<Row>{};
}

BatteryState battery_state(std::optional<std::uint16_t> status) {
  switch (status.value_or(0)) {
    case 1:  // discharging
    case 4:  // low
    case 5:  // critical
      return BatteryState::discharging;
    case 2:   // on AC power, not necessarily charging
    case 11:  // partially charged
      return BatteryState::not_charging;
    case 3:
      return BatteryState::full;
    case 6:  // charging
    case 7:  // charging and high
    case 8:  // charging and low
    case 9:  // charging and critical
      return BatteryState::charging;
    default:
      return BatteryState::unknown;
  }
}

}  // namespace

result<std::vector<Battery>> batteries() {
  const auto rows =
      internal::wmi::query("Win32_Battery", {"Name", "Chemistry", "DesignCapacity", "FullChargeCapacity"});
  if (!rows) {
    return std::unexpected(rows.error());
  }
  const auto static_data = acpi_rows(
      "BatteryStaticData", {"ManufactureName", "DeviceName", "SerialNumber", "DesignedCapacity"}, rows->size());
  const auto full_charged = acpi_rows("BatteryFullChargedCapacity", {"FullChargedCapacity"}, rows->size());

  std::vector<Battery> result;
  result.reserve(rows->size());
  for (std::size_t i = 0; i < rows->size(); ++i) {
    const Row& battery = (*rows)[i];
    const Row empty;
    const Row& acpi = i < static_data.size() ? static_data[i] : empty;
    const Row& full = i < full_charged.size() ? full_charged[i] : empty;
    result.push_back(Battery{
        .index = static_cast<std::uint32_t>(i),
        .vendor = acpi.string("ManufactureName"),
        .model = acpi.string("DeviceName").or_else([&] { return battery.string("Name"); }),
        .serial_number = acpi.string("SerialNumber"),
        .technology = technology(battery.number<std::uint16_t>("Chemistry")),
        .design_capacity_wh = watt_hours(battery.number<std::uint32_t>("DesignCapacity")).or_else([&] {
          return watt_hours(acpi.number<std::uint32_t>("DesignedCapacity"));
        }),
        .full_charge_capacity_wh = watt_hours(battery.number<std::uint32_t>("FullChargeCapacity")).or_else([&] {
          return watt_hours(full.number<std::uint32_t>("FullChargedCapacity"));
        }),
    });
  }
  return result;
}

result<BatteryStatus> battery_status(std::uint32_t index) {
  const auto rows = internal::wmi::query("Win32_Battery", {"BatteryStatus", "EstimatedChargeRemaining"});
  if (!rows) {
    return std::unexpected(rows.error());
  }
  if (index >= rows->size()) {
    return std::unexpected(error{errc::not_found, std::format("battery {}", index)});
  }
  const Row& battery = (*rows)[index];
  return BatteryStatus{
      .state = battery_state(battery.number<std::uint16_t>("BatteryStatus")),
      .charge = battery.number<std::uint16_t>("EstimatedChargeRemaining").transform([](std::uint16_t percent) {
        return std::clamp(percent / 100.0, 0.0, 1.0);
      }),
  };
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
