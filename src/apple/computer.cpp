// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/computer.h>

#include <optional>
#include <string>
#include <string_view>

#include "internal/apple_cf.h"
#include "internal/apple_models.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;

// Marketing name, e.g. "MacBook Pro (14-inch, 2021)".
// Reported by Apple Silicon Macs, looked up by model identifier for Intel Macs.
std::optional<std::string> marketing_name(const std::optional<std::string>& model) {
  const cf::io_ptr product(IORegistryEntryFromPath(MACH_PORT_NULL, "IODeviceTree:/product"));
  if (auto name = product ? cf::string_property(product.get(), CFSTR("product-name")) : std::nullopt) {
    return name;
  }
  if (const auto name = model ? cf::mac_marketing_name(*model) : std::nullopt) {
    return std::string(*name);
  }
  return std::nullopt;
}

// From the marketing name if available, otherwise from the model identifier.
// Newer identifiers ("Mac14,2") do not tell the product line.
ChassisType chassis(const std::optional<std::string>& name, const std::optional<std::string>& model) {
  const std::string_view s = name ? *name : model ? *model : std::string_view{};
  if (s.starts_with("MacBook")) {
    return ChassisType::laptop;
  }
  if (s.starts_with("iMac")) {
    return ChassisType::all_in_one;
  }
  if (s.starts_with("Mac mini") || s.starts_with("Macmini")) {
    return ChassisType::mini_pc;
  }
  if (s.starts_with("Mac Pro") || s.starts_with("MacPro") || s.starts_with("Mac Studio")) {
    return ChassisType::desktop;
  }
  return ChassisType::unknown;
}

}  // namespace

result<Computer> computer() {
  const cf::io_ptr platform(IOServiceGetMatchingService(MACH_PORT_NULL, IOServiceMatching("IOPlatformExpertDevice")));
  if (!platform) {
    return std::unexpected(error{errc::not_found, "IOPlatformExpertDevice"});
  }
  auto model = cf::string_property(platform.get(), CFSTR("model"));  // e.g. "MacBookPro18,3"
  auto family = marketing_name(model);
  const ChassisType type = chassis(family, model);
  return Computer{
      .vendor = cf::string_property(platform.get(), CFSTR("manufacturer")),  // "Apple Inc."
      .model = std::move(model),
      .family = std::move(family),
      .version = std::nullopt,
      .sku = std::nullopt,
      .serial_number = cf::string_property(platform.get(), CFSTR(kIOPlatformSerialNumberKey)),
      .chassis = type,
  };
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
