// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/mainboard.h>

#include "internal/apple_cf.h"

namespace hwinfo {

result<Mainboard> mainboard() {
  namespace cf = internal::apple;
  const cf::io_ptr platform(IOServiceGetMatchingService(MACH_PORT_NULL, IOServiceMatching("IOPlatformExpertDevice")));
  if (!platform) {
    return std::unexpected(error{errc::not_found, "IOPlatformExpertDevice"});
  }
  return Mainboard{
      .vendor = cf::string_property(platform.get(), CFSTR("manufacturer")),  // "Apple Inc."
      .name = cf::string_property(platform.get(), CFSTR("model")),           // e.g. "MacBookPro18,3"
      .version = std::nullopt,
      .serial_number = cf::string_property(platform.get(), CFSTR(kIOPlatformSerialNumberKey)),
  };
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
