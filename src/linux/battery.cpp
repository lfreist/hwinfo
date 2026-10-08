// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/battery.h>
#include <hwinfo/monitoring.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <vector>

#include "internal/file.h"

namespace hwinfo {

namespace {

const std::filesystem::path power_supply = "/sys/class/power_supply";

// Battery directories in /sys/class/power_supply, sorted by name (BAT0, BAT1, ...).
result<std::vector<std::filesystem::path>> battery_paths() {
  std::error_code ec;
  std::vector<std::filesystem::path> paths;
  for (const auto& entry : std::filesystem::directory_iterator(power_supply, ec)) {
    if (internal::read_attribute(entry.path() / "type") == "Battery") {
      paths.push_back(entry.path());
    }
  }
  if (ec && ec != std::errc::no_such_file_or_directory) {
    return std::unexpected(error{ec, power_supply.string()});
  }
  std::ranges::sort(paths);
  return paths;
}

// Energy in Wh from energy_* (µWh) or charge_* (µAh, converted with the design voltage) attributes.
std::optional<double> read_wh(const std::filesystem::path& path, std::string_view suffix) {
  if (const auto energy = internal::read_number_attribute<double>(path / std::format("energy_{}", suffix))) {
    return *energy / 1e6;
  }
  const auto charge = internal::read_number_attribute<double>(path / std::format("charge_{}", suffix));
  const auto voltage = internal::read_number_attribute<double>(path / "voltage_min_design");
  if (charge && voltage) {
    return *charge * *voltage / 1e12;
  }
  return std::nullopt;
}

}  // namespace

result<std::vector<Battery>> batteries() {
  return battery_paths().transform([](const std::vector<std::filesystem::path>& paths) {
    std::vector<Battery> result;
    for (const auto& path : paths) {
      result.push_back(Battery{
          .index = static_cast<std::uint32_t>(result.size()),
          .vendor = internal::read_attribute(path / "manufacturer"),
          .model = internal::read_attribute(path / "model_name"),
          .serial_number = internal::read_attribute(path / "serial_number"),
          .technology = internal::read_attribute(path / "technology"),
          .design_capacity_wh = read_wh(path, "full_design"),
          .full_charge_capacity_wh = read_wh(path, "full"),
      });
    }
    return result;
  });
}

result<BatteryStatus> battery_status(std::uint32_t index) {
  const auto paths = battery_paths();
  if (!paths) {
    return std::unexpected(paths.error());
  }
  if (index >= paths->size()) {
    return std::unexpected(error{errc::not_found, std::format("battery {}", index)});
  }
  const auto& path = (*paths)[index];

  BatteryStatus status;
  const auto state = internal::read_attribute(path / "status").value_or("Unknown");
  if (state == "Charging") {
    status.state = BatteryState::charging;
  } else if (state == "Discharging") {
    status.state = BatteryState::discharging;
  } else if (state == "Full") {
    status.state = BatteryState::full;
  } else if (state == "Not charging") {
    status.state = BatteryState::not_charging;
  }

  if (const auto percent = internal::read_number_attribute<double>(path / "capacity")) {
    status.charge = *percent / 100.0;
  } else if (const auto now = read_wh(path, "now"), full = read_wh(path, "full"); now && full && *full > 0) {
    status.charge = std::clamp(*now / *full, 0.0, 1.0);
  }
  return status;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
