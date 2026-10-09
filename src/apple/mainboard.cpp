// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/mainboard.h>

#include <optional>
#include <string>
#include <string_view>

#include "internal/apple_cf.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;

// Board identifier: "board-id" on Intel Macs (e.g. "Mac-06F11FD93F0323C5").
// On Apple Silicon, the first entry of the NUL-separated "compatible" list that is neither the model identifier nor the
// generic "AppleARM" (e.g. "J316sAP").
std::optional<std::string> board_name(io_registry_entry_t platform) {
  if (auto board_id = cf::string_property(platform, CFSTR("board-id"))) {
    return board_id;
  }
  const auto compatible = cf::property(platform, CFSTR("compatible"));
  if (!compatible || CFGetTypeID(compatible.get()) != CFDataGetTypeID()) {
    return std::nullopt;
  }
  const auto data = static_cast<CFDataRef>(compatible.get());
  const std::string_view entries(reinterpret_cast<const char*>(CFDataGetBytePtr(data)),
                                 static_cast<std::size_t>(CFDataGetLength(data)));
  const auto model = cf::string_property(platform, CFSTR("model"));
  for (const auto entry : internal::split(entries, '\0')) {
    if (!entry.empty() && entry != model && entry != "AppleARM") {
      return std::string(entry);
    }
  }
  return std::nullopt;
}

}  // namespace

result<Mainboard> mainboard() {
  const cf::io_ptr platform(IOServiceGetMatchingService(MACH_PORT_NULL, IOServiceMatching("IOPlatformExpertDevice")));
  if (!platform) {
    return std::unexpected(error{errc::not_found, "IOPlatformExpertDevice"});
  }
  return Mainboard{
      .vendor = cf::string_property(platform.get(), CFSTR("manufacturer")),  // "Apple Inc."
      .name = board_name(platform.get()),
      .version = std::nullopt,
      .serial_number = cf::string_property(platform.get(), CFSTR(kIOPlatformSerialNumberKey)),
  };
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
