// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// APFS naming. Pure functions, so they can be unit tested on any platform. Not part of the public API.

#pragma once

#include <string>
#include <string_view>

namespace hwinfo::internal {

// The APFS container (its synthesized disk) of an APFS volume's BSD name: "disk3s1" -> "disk3", snapshot "disk3s1s1"
// -> "disk3". Names that are no "disk<N>..." are returned unchanged.
inline std::string apfs_container(std::string_view bsd_name) {
  if (!bsd_name.starts_with("disk")) {
    return std::string(bsd_name);
  }
  const auto end = bsd_name.find_first_not_of("0123456789", 4);
  return std::string(bsd_name.substr(0, end == 4 ? bsd_name.size() : end));
}

}  // namespace hwinfo::internal
