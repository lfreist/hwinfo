
// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_UNIX

#include <hwinfo/PCIMapper.h>
#include <hwinfo/gpu.h>
#include <hwinfo/utils/filesystem.h>

#include <fstream>
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
  try {
    freqs[0] = std::stoi(read_drm_by_path(drm_path + "gt_min_freq_mhz"));
  } catch (const std::invalid_argument& e) {
    freqs[0] = -1;
  }
  try {
    freqs[1] = std::stoi(read_drm_by_path(drm_path + "gt_cur_freq_mhz"));
  } catch (const std::invalid_argument& e) {
    freqs[0] = -1;
  }
  try {
    freqs[2] = std::stoi(read_drm_by_path(drm_path + "gt_max_freq_mhz"));
  } catch (const std::invalid_argument& e) {
    freqs[0] = -1;
  }
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
    gpu._frequency_MHz = frequencies[2];
    gpus.push_back(std::move(gpu));
    id++;
  }
#ifdef USE_OCL
  auto cl_gpus = get_cpu_cl_data();
  if (cl_gpus.size() == gpus.size()) {
    for (int i = 0; i < cl_gpus.size(); ++i) {
      gpus[i]._memory_Bytes = cl_gpus[i].memory_Bytes;
      gpus[i]._num_cores = cl_gpus[i].num_cores;
      gpus[i]._driverVersion = cl_gpus[i].driver_version;
    }
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX