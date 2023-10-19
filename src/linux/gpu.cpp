
// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/gpu.h>
#include <hwinfo/utils/PCIMapper.h>
#include <hwinfo/utils/filesystem.h>

#ifdef USE_OCL
#include <missocl/opencl.h>
#endif

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
      if (id > 2) {
        break;
      }
      id++;
      continue;
    }
    gpu._vendor_id = read_drm_by_path(path + "device/vendor");
    gpu._device_id = read_drm_by_path(path + "device/device");
    if (gpu._vendor_id.empty() || gpu._device_id.empty()) {
      id++;
      continue;
    }
    const PCIVendor& vendor = pci[gpu._vendor_id];
    const PCIDevice device = vendor[gpu._device_id];
    gpu._vendor = vendor.vendor_name;
    gpu._name = vendor[gpu._device_id].device_name;
    auto frequencies = get_frequencies(path);
    gpu._frequency_MHz = frequencies[2];
    gpus.push_back(std::move(gpu));
    id++;
  }
#ifdef USE_OCL
  auto cl_gpus = mcl::DeviceManager::get_list<mcl::Filter::GPU>();
  for (auto& gpu : gpus) {
    for (auto* cl_gpu : cl_gpus) {
      if (cl_gpu->name().find(gpu._device_id)) {
        gpu._driverVersion = cl_gpu->driver_version();
        gpu._frequency_MHz = static_cast<int64_t>(cl_gpu->clock_frequency_MHz());
        gpu._num_cores = static_cast<int>(cl_gpu->cores());
        gpu._memory_Bytes = static_cast<int64_t>(cl_gpu->memory_Bytes());
      }
    }
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX