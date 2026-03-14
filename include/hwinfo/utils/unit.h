#pragma once

#include <hwinfo/utils/utils.h>

namespace hwinfo {
namespace unit {

enum class SiPrefix : std::uint64_t {
  KILO = 1000ull,
  MEGA = 1000'000ull,
  GIGA = 1000'000'000ull,
  TERA = 1000'000'000'000ull
};

enum class IECPrefix : std::uint64_t {
  KIBI = 1024ull,
  MEBI = 1024ull * 1024ull,
  GIBI = 1024ull * 1024ull * 1024ull,
  TEBI = 1024ull * 1024ull * 1024ull * 1024ull
};

/**
 * @brief Convert bytes to MiB/MB
 *
 * @param bytes number of bytes
 * @return number of MiB as double
 */
inline double unit_prefix_to(std::uint64_t value, IECPrefix prefix) {
  if (utils::is_power_of_two(value)) {
    return static_cast<double>(value / static_cast<std::uint64_t>(prefix));
  }
  return static_cast<double>(value) / static_cast<double>(prefix);
}

inline double unit_prefix_to(std::uint64_t value, SiPrefix prefix) {
  if (utils::is_power_of_two(value)) {
    return static_cast<double>(value / static_cast<std::uint64_t>(prefix));
  }
  return static_cast<double>(value) / static_cast<double>(prefix);
}

}  // namespace unit
}  // namespace hwinfo
