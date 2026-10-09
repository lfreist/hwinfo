// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/battery.h>
#include <hwinfo/cpu.h>
#include <hwinfo/detail/formatter.h>
#include <hwinfo/disk.h>
#include <hwinfo/error.h>
#include <hwinfo/gpu.h>
#include <hwinfo/mainboard.h>
#include <hwinfo/network.h>
#include <hwinfo/os.h>
#include <hwinfo/platform.h>
#include <hwinfo/ram.h>

#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo {

enum class ChassisType { unknown, desktop, laptop, tablet, all_in_one, mini_pc, server, other };

// The computer as a product (e.g. a notebook or prebuilt PC), as opposed to its mainboard.
// The member functions give access to the installed components. Each one queries on call and requires linking the
// corresponding component (e.g. lfreist-hwinfo::cpu for cpus()).
struct Computer {
  std::optional<std::string> vendor{};  // e.g. "LENOVO", "Apple Inc."
  std::optional<std::string> model{};   // e.g. "21CBCTO1WW", "MacBookPro18,3"
  std::optional<std::string> family{};  // e.g. "ThinkPad X1 Carbon Gen 10"
  std::optional<std::string> version{};
  std::optional<std::string> sku{};
  std::optional<std::string> serial_number{};  // usually requires elevated privileges on Linux
  ChassisType chassis = ChassisType::unknown;

  [[nodiscard]] result<Os> os() const { return hwinfo::os(); }
  [[nodiscard]] result<Mainboard> mainboard() const { return hwinfo::mainboard(); }
  [[nodiscard]] result<std::vector<Cpu>> cpus() const { return hwinfo::cpus(); }
  [[nodiscard]] result<Memory> memory() const { return hwinfo::memory(); }
  [[nodiscard]] result<std::vector<Gpu>> gpus() const { return hwinfo::gpus(); }
  [[nodiscard]] result<std::vector<Disk>> disks() const { return hwinfo::disks(); }
  [[nodiscard]] result<std::vector<Battery>> batteries() const { return hwinfo::batteries(); }
  [[nodiscard]] result<std::vector<NetworkInterface>> network_interfaces() const {
    return hwinfo::network_interfaces();
  }

  friend bool operator==(const Computer&, const Computer&) = default;
};

[[nodiscard]] HWINFO_API result<Computer> computer();

constexpr std::string_view to_string(ChassisType chassis) noexcept {
  switch (chassis) {
    case ChassisType::desktop:
      return "desktop";
    case ChassisType::laptop:
      return "laptop";
    case ChassisType::tablet:
      return "tablet";
    case ChassisType::all_in_one:
      return "all-in-one";
    case ChassisType::mini_pc:
      return "mini PC";
    case ChassisType::server:
      return "server";
    case ChassisType::other:
      return "other";
    case ChassisType::unknown:
      break;
  }
  return "unknown";
}

inline std::string to_string(const Computer& computer) {
  return std::format("{} {}", computer.vendor.value_or("unknown vendor"), computer.model.value_or("unknown model"));
}

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::ChassisType> : hwinfo::detail::to_string_formatter<hwinfo::ChassisType> {};

template <>
struct std::formatter<hwinfo::Computer> : hwinfo::detail::to_string_formatter<hwinfo::Computer> {};
