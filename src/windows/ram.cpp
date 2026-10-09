// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/ram.h>
#include <windows.h>

#include <cstdint>
#include <vector>

#include "internal/windows_error.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

// Installed modules from SMBIOS (via WMI). Empty if WMI is not available.
std::vector<MemoryModule> memory_modules() {
  const auto rows =
      internal::wmi::query("Win32_PhysicalMemory", {"BankLabel", "Capacity", "ConfiguredClockSpeed", "DeviceLocator",
                                                    "Manufacturer", "PartNumber", "SerialNumber", "Speed"});
  if (!rows) {
    return {};
  }
  std::vector<MemoryModule> modules;
  modules.reserve(rows->size());
  for (const auto& row : *rows) {
    const auto mhz =
        row.number<std::uint64_t>("ConfiguredClockSpeed").or_else([&] { return row.number<std::uint64_t>("Speed"); });
    modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(modules.size()),
        .vendor = row.string("Manufacturer"),
        .name = row.string("DeviceLocator").or_else([&] { return row.string("BankLabel"); }),
        .model = row.string("PartNumber"),
        .serial_number = row.string("SerialNumber"),
        .size = row.number<std::uint64_t>("Capacity").transform([](std::uint64_t bytes) { return Bytes{bytes}; }),
        .frequency = mhz.and_then(
            [](std::uint64_t value) { return value > 0 ? std::optional(value * FrequencyUnit::MHz) : std::nullopt; }),
    });
  }
  return modules;
}

}  // namespace

result<Memory> memory() {
  MEMORYSTATUSEX status{};
  status.dwLength = sizeof(status);
  if (!GlobalMemoryStatusEx(&status)) {
    return std::unexpected(internal::last_error("GlobalMemoryStatusEx"));
  }
  return Memory{.total = {status.ullTotalPhys}, .modules = memory_modules()};
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
