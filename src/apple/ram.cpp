// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <IOKit/IOKitLib.h>
#include <hwinfo/ram.h>

#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "internal/apple_cf.h"
#include "internal/smbios.h"
#include "internal/sysctl.h"

namespace hwinfo {

namespace {

namespace cf = internal::apple;

// The unified memory of Apple Silicon as one soldered module, from the device tree properties iBoot sets on
// IODeviceTree:/chosen: "dram-type" (e.g. "LPDDR5"), "dram-vendor" (e.g. "Samsung") and "dram-size" (bytes).
// std::nullopt if they are missing (Intel Macs, virtual machines).
std::optional<std::vector<MemoryModule>> unified_memory(std::uint64_t total) {
  const cf::io_ptr chosen(IORegistryEntryFromPath(MACH_PORT_NULL, "IODeviceTree:/chosen"));
  if (!chosen) {
    return std::nullopt;
  }
  auto type = cf::to_string(cf::property(chosen.get(), CFSTR("dram-type")).get());
  if (!type) {
    return std::nullopt;
  }
  const auto size = cf::to_number<std::uint64_t>(cf::property(chosen.get(), CFSTR("dram-size")).get());
  return std::vector<MemoryModule>{{
      .index = 0,
      .vendor = cf::to_string(cf::property(chosen.get(), CFSTR("dram-vendor")).get()),
      .size = Bytes{size.value_or(total)},
      .type = std::move(type),
      .form_factor = MemoryFormFactor::soldered,  // in the SoC package
  }};
}

}  // namespace

result<Memory> memory() {
  return internal::sysctl_value<std::uint64_t>("hw.memsize").transform([](std::uint64_t total) {
    // Modules and slots from SMBIOS on Intel Macs. Apple Silicon has no SMBIOS: its memory is in the SoC package.
    const auto& table = internal::system_table();
    Memory memory = table ? internal::memory_info(*table) : Memory{};
    if (!memory.modules) {
      memory.modules = unified_memory(total);
    }
    memory.total = Bytes{total};
    return memory;
  });
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
