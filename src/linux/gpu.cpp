
// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <hwinfo/PCIMapper.h>
#include <hwinfo/gpu.h>
#include <hwinfo/utils/filesystem.h>
#include <hwinfo/utils/stringutils.h>

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <vector>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string read_drm_by_path(const std::string& path) {
  std::ifstream f_drm(path);
  if (!f_drm) {
    return "";
  }
  std::string ret;
  getline(f_drm, ret);
  return ret;
}

// _____________________________________________________________________________________________________________________
std::vector<int> get_frequencies(const std::string drm_path) {
  // {min, current, max}
  std::vector<int> freqs(3);
  freqs[0] = std::stoi(read_drm_by_path(drm_path + "gt_min_freq_mhz"));
  freqs[1] = std::stoi(read_drm_by_path(drm_path + "gt_cur_freq_mhz"));
  freqs[2] = std::stoi(read_drm_by_path(drm_path + "gt_max_freq_mhz"));
  return freqs;
}

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  std::vector<GPU> gpus{};
  PCIMapper pci = PCI::getMapper();
  int id = 0;
  while (true) {
    GPU gpu;
    gpu._id = id;
    std::string path("/sys/class/drm/card" + std::to_string(id) + '/');
    if (!filesystem::exists(path)) {
      break;
    }
    std::string vendor_id = read_drm_by_path(path + "device/vendor");
    std::string device_id = read_drm_by_path(path + "device/device");
    const PCIVendor& vendor = pci[vendor_id];
    const PCIDevice device = vendor[device_id];
    gpu._vendor = vendor.vendor_name;
    gpu._name = vendor[device_id].device_name;
    auto frequencies = get_frequencies(path);
    gpu._minFrequency_MHz = frequencies[0];
    gpu._currentFrequency_MHz = frequencies[1];
    gpu._maxFrequency_MHz = frequencies[2];
    gpus.push_back(std::move(gpu));
    id++;
  }
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX