#pragma once

#include <cmath>

namespace hwinfo {
namespace unit {

const auto mib_bytes = std::pow(2, 20);  // NOLINT(cert-err58-cpp)

/**
 * @brief Convert bytes to MiB
 *
 * @param bytes number of bytes
 * @return number of MiB as double
 */
inline double bytes_to_MiB(size_t bytes) { return static_cast<double>(bytes) / mib_bytes; }

}  // namespace unit
}  // namespace hwinfo
