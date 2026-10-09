// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/computer.h>

#include <cstdint>
#include <filesystem>
#include <string>

#include "internal/dmi.h"
#include "internal/file.h"

namespace hwinfo {

result<Computer> computer() {
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
        .chassis = chassis ? internal::chassis_from_smbios(*chassis) : ChassisType::unknown,
    };
  }
  // Devices without SMBIOS (e.g. ARM boards) describe themselves in the device tree.
  if (const auto model = internal::read_file("/proc/device-tree/model")) {
    if (auto name = internal::non_empty(std::string_view(*model).substr(0, model->find('\0')))) {
      return Computer{.model = std::move(name)};
    }
  }
  return std::unexpected(error{errc::not_supported, "no DMI or device tree information available"});
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
