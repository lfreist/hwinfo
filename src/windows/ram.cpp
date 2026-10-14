// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/ram.h>
#include <windows.h>

#include <cstdint>
#include <vector>

#include "internal/smbios.h"
#include "internal/windows_error.h"
#include "internal/wmi_wrapper.h"

namespace hwinfo {

namespace {

// Win32_PhysicalMemory.FormFactor (CIM values, not the SMBIOS ones)
MemoryFormFactor form_factor(std::optional<std::uint16_t> value) {
  switch (value.value_or(0)) {
    case 0:
      return MemoryFormFactor::unknown;
    case 8:   // DIMM
    case 11:  // RIMM
      return MemoryFormFactor::dimm;
    case 12:  // SODIMM
    case 13:  // SRIMM
      return MemoryFormFactor::sodimm;
    case 9:  // TSOP
      return MemoryFormFactor::soldered;
    default:
      return MemoryFormFactor::other;
  }
}

std::optional<std::uint32_t> positive(std::optional<std::uint32_t> value) {
  return value.and_then([](std::uint32_t v) { return v > 0 ? std::optional(v) : std::nullopt; });
}

// SMBIOS as decoded by WMI. Fallback if the raw table cannot be read; empty if WMI is not available.
Memory memory_from_wmi() {
  Memory memory;
  if (const auto arrays =
          internal::wmi::query("Win32_PhysicalMemoryArray", {"Use", "MemoryDevices", "MaxCapacityEx"})) {
    for (const auto& array : *arrays) {
      if (array.number<std::uint16_t>("Use") != 3) {  // system memory
        continue;
      }
      memory.slots = memory.slots.value_or(0) + array.number<std::uint32_t>("MemoryDevices").value_or(0);
      if (const auto kib = array.number<std::uint64_t>("MaxCapacityEx")) {
        memory.max_capacity = memory.max_capacity.value_or(Bytes{}) + *kib * ByteUnit::KiB;
      }
    }
  }
  const auto rows = internal::wmi::query(
      "Win32_PhysicalMemory",
      {"Attributes", "BankLabel", "Capacity", "ConfiguredClockSpeed", "ConfiguredVoltage", "DataWidth", "DeviceLocator",
       "FormFactor", "Manufacturer", "PartNumber", "SerialNumber", "SMBIOSMemoryType", "Speed", "TotalWidth"});
  if (!rows) {
    return memory;
  }
  memory.modules.reserve(rows->size());
  for (const auto& row : *rows) {
    const auto speed = internal::mts_frequency(row.number<std::uint32_t>("Speed"));
    memory.modules.push_back(MemoryModule{
        .index = static_cast<std::uint32_t>(memory.modules.size()),
        .vendor = row.string("Manufacturer"),
        .name = row.string("DeviceLocator").or_else([&] { return row.string("BankLabel"); }),
        .model = row.string("PartNumber"),
        .serial_number = row.string("SerialNumber"),
        .size = row.number<std::uint64_t>("Capacity").transform([](std::uint64_t bytes) { return Bytes{bytes}; }),
        .frequency =
            internal::mts_frequency(row.number<std::uint32_t>("ConfiguredClockSpeed")).or_else([&] { return speed; }),
        .max_frequency = speed,
        .type = row.number<std::uint8_t>("SMBIOSMemoryType").and_then(internal::memory_type_name),
        .form_factor = form_factor(row.number<std::uint16_t>("FormFactor")),
        .rank = positive(row.number<std::uint32_t>("Attributes").transform([](std::uint32_t a) { return a & 0x0f; })),
        .data_width = positive(row.number<std::uint32_t>("DataWidth")),
        .total_width = positive(row.number<std::uint32_t>("TotalWidth")),
        .voltage_mv = positive(row.number<std::uint32_t>("ConfiguredVoltage")),
    });
  }
  return memory;
}

}  // namespace

result<Memory> memory() {
  MEMORYSTATUSEX status{};
  status.dwLength = sizeof(status);
  if (!GlobalMemoryStatusEx(&status)) {
    return std::unexpected(internal::last_error("GlobalMemoryStatusEx"));
  }
  // GetSystemFirmwareTable needs no administrator rights; WMI only if it fails
  const auto& table = internal::system_table();
  Memory memory = table ? internal::memory_info(*table) : memory_from_wmi();
  memory.total = Bytes{status.ullTotalPhys};
  return memory;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
