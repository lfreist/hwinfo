// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/computer.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <string>

#include "internal/dmi.h"
#include "internal/file.h"
#include "internal/procfs.h"
#include "internal/uuid.h"

namespace hwinfo {

namespace {

// Identity from DMI or, on devices without SMBIOS (e.g. ARM boards), the device tree.
Computer identity() {
  if (const auto dmi = internal::dmi_directory()) {
    const auto read = [&](std::string_view name) {
      return internal::dmi_string(internal::read_attribute(*dmi / name));
    };
    const auto chassis = internal::read_number_attribute<std::uint32_t>(*dmi / "chassis_type");
    return Computer{
        .vendor = read("sys_vendor"),
        .model = read("product_name"),
        .family = read("product_family"),
        .version = read("product_version"),
        .sku = read("product_sku"),
        .serial_number = read("product_serial"),
        .uuid = internal::system_uuid(internal::read_attribute(*dmi / "product_uuid").and_then(internal::parse_uuid)),
        .chassis = chassis ? internal::chassis_from_smbios(*chassis) : ChassisType::unknown,
        .chassis_code = chassis ? std::optional(static_cast<std::uint8_t>(*chassis & 0x7f)) : std::nullopt,
    };
  }
  if (const auto model = internal::read_file("/proc/device-tree/model")) {
    return Computer{.model = internal::non_empty(std::string_view(*model).substr(0, model->find('\0')))};
  }
  return Computer{};
}

}  // namespace

result<Computer> computer() {
  Computer computer = identity();
  // btime is the boot time the kernel keeps, independent of when it is read (now - uptime is off by up to a second)
  if (const auto btime =
          internal::read_file("/proc/stat").transform(internal::procfs::parse_btime).value_or(std::nullopt)) {
    computer.boot_time = std::chrono::sys_seconds(std::chrono::seconds(*btime));
  }
  return computer;
}

result<std::chrono::seconds> uptime() {
  const auto content = internal::read_file("/proc/uptime");
  if (!content) {
    return std::unexpected(content.error());
  }
  if (const auto seconds = internal::procfs::parse_uptime(*content)) {
    return *seconds;
  }
  return std::unexpected(error{errc::parse_error, "/proc/uptime"});
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
