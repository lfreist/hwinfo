// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <regex>
#include <string>
#include <vector>

#include "hwinfo/gpu.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string read_drm_by_path(const std::string& path) {
  // TODO: implement
  return "<unknown>";
}

// _____________________________________________________________________________________________________________________
std::vector<int> get_frequencies(const std::string drm_path) {
  std::vector<int> freqs(3);
  // TODO: implement
  return freqs;
}

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  std::vector<GPU> gpus{};
  // TODO: implement
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE