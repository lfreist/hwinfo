// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/battery.h>
#include <hwinfo/monitoring.h>

#include <algorithm>
#include <cstdint>
#include <format>
#include <optional>
#include <vector>

#include "internal/apple_cf.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;

result<std::vector<cf::io_ptr>> smart_batteries() { return cf::matching_services("AppleSmartBattery"); }

// mAh * mV -> Wh
std::optional<double> watt_hours(std::optional<std::int64_t> mah, std::optional<std::int64_t> mv) {
  if (!mah || !mv || *mah <= 0 || *mv <= 0) {
    return std::nullopt;
  }
  return static_cast<double>(*mah) * static_cast<double>(*mv) / 1e6;
}

// Full charge capacity in mAh. "MaxCapacity" is given in mAh on Intel Macs but in percent on Apple Silicon.
std::optional<std::int64_t> full_charge_mah(io_registry_entry_t battery) {
  if (auto raw = cf::number_property<std::int64_t>(battery, CFSTR("AppleRawMaxCapacity"))) {
    return raw;
  }
  if (auto nominal = cf::number_property<std::int64_t>(battery, CFSTR("NominalChargeCapacity"))) {
    return nominal;
  }
  const auto max = cf::number_property<std::int64_t>(battery, CFSTR("MaxCapacity"));
  return max && *max > 100 ? max : std::nullopt;
}

}  // namespace

result<std::vector<Battery>> batteries() {
  return smart_batteries().transform([](const std::vector<cf::io_ptr>& services) {
    std::vector<Battery> result;
    for (const auto& service : services) {
      const io_registry_entry_t battery = service.get();
      const auto voltage = cf::number_property<std::int64_t>(battery, CFSTR("Voltage"));  // mV
      auto serial = cf::string_property(battery, CFSTR("Serial"));
      if (!serial) {
        serial = cf::string_property(battery, CFSTR("BatterySerialNumber"));
      }
      result.push_back(Battery{
          .index = static_cast<std::uint32_t>(result.size()),
          .vendor = cf::string_property(battery, CFSTR("Manufacturer")),
          .model = cf::string_property(battery, CFSTR("DeviceName")),
          .serial_number = std::move(serial),
          .technology = std::nullopt,
          .design_capacity_wh =
              watt_hours(cf::number_property<std::int64_t>(battery, CFSTR("DesignCapacity")), voltage),
          .full_charge_capacity_wh = watt_hours(full_charge_mah(battery), voltage),
      });
    }
    return result;
  });
}

result<BatteryStatus> battery_status(std::uint32_t index) {
  const auto services = smart_batteries();
  if (!services) {
    return std::unexpected(services.error());
  }
  if (index >= services->size()) {
    return std::unexpected(error{errc::not_found, std::format("battery {}", index)});
  }
  const io_registry_entry_t battery = (*services)[index].get();

  BatteryStatus status;
  const auto fully_charged = cf::bool_property(battery, CFSTR("FullyCharged"));
  const auto charging = cf::bool_property(battery, CFSTR("IsCharging"));
  const auto external = cf::bool_property(battery, CFSTR("ExternalConnected"));
  if (fully_charged == true) {
    status.state = BatteryState::full;
  } else if (charging == true) {
    status.state = BatteryState::charging;
  } else if (external == true) {
    status.state = BatteryState::not_charging;  // on AC, but charging is paused (e.g. optimized charging)
  } else if (external == false) {
    status.state = BatteryState::discharging;
  }

  // Both in percent (Apple Silicon) or both in mAh (Intel).
  const auto current = cf::number_property<std::int64_t>(battery, CFSTR("CurrentCapacity"));
  const auto max = cf::number_property<std::int64_t>(battery, CFSTR("MaxCapacity"));
  if (current && max && *max > 0) {
    status.charge = std::clamp(static_cast<double>(*current) / static_cast<double>(*max), 0.0, 1.0);
  }
  return status;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
