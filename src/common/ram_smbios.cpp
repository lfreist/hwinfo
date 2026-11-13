// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/ram.h>

#include <cstddef>
#include <span>
#include <utility>

#include "internal/smbios.h"

namespace hwinfo {

result<Memory> memory_from_smbios(std::span<const std::byte> entry_point, std::span<const std::byte> table) {
  return internal::table_from_bytes(entry_point, table).and_then([](const smbios::Table& parsed) -> result<Memory> {
    if (auto memory = internal::decode_memory(parsed, true)) {
      return *std::move(memory);  // total stays 0
    }
    return std::unexpected(error(errc::parse_error, "SMBIOS memory structure"));
  });
}

}  // namespace hwinfo
