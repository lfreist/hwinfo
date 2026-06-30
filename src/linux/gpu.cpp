// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include "hwinfo/gpu.h"
#include "hwinfo/utils/PCIMapper.h"
#include "hwinfo/utils/unit.h"

#ifdef USE_OCL
#include <hwinfo/opencl/device.h>
#endif

#include <algorithm>
#include <filesystem>
#include <fstream>
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
std::vector<std::string> read_drm_lines_by_path(const std::string& path) {
  std::ifstream f_drm(path);
  std::vector<std::string> lines;
  if (!f_drm) {
    return lines;
  }
  std::string line;
  while (getline(f_drm, line)) {
    lines.push_back(line);
  }
  return lines;
}

// _____________________________________________________________________________________________________________________
// Reads a "*_mhz" sysfs attribute and returns it in Hz (0 on missing/garbage).
std::uint64_t read_freq_mhz_as_hz(const std::filesystem::path& path) {
  try {
    return std::stoull(read_drm_by_path(path)) * static_cast<std::uint64_t>(unit::SiPrefix::MEGA);
  } catch (...) {
    return 0;
  }
}

// _____________________________________________________________________________________________________________________
// Reads a sysfs attribute that is already expressed in Hz (0 on missing/garbage).
std::uint64_t read_freq_hz(const std::filesystem::path& path) {
  try {
    return std::stoull(read_drm_by_path(path));
  } catch (...) {
    return 0;
  }
}

// _____________________________________________________________________________________________________________________
// First subdirectory of `dir`, or an empty path. Used for the wildcard-named
// nodes under device/hwmon/hwmon# and device/devfreq/<node>.
std::filesystem::path first_subdirectory(const std::filesystem::path& dir) {
  std::error_code ec;
  if (std::filesystem::is_directory(dir, ec)) {
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
      if (entry.is_directory()) {
        return entry.path();
      }
    }
  }
  return {};
}

// _____________________________________________________________________________________________________________________
// Intel i915: gt_*_freq_mhz directly under the card directory. On multi-tile
// parts these are a deprecated alias for tile 0 (gt/gt0/rps_*_freq_mhz).
std::vector<std::uint64_t> get_intel_i915_frequencies(const std::filesystem::path& drm_path) {
  // {min, current, max}
  std::vector<std::uint64_t> freqs(3, 0);
  freqs[0] = read_freq_mhz_as_hz(drm_path / "gt_min_freq_mhz");
  freqs[1] = read_freq_mhz_as_hz(drm_path / "gt_cur_freq_mhz");
  freqs[2] = read_freq_mhz_as_hz(drm_path / "gt_max_freq_mhz");
  return freqs;
}

// _____________________________________________________________________________________________________________________
// Intel Xe: per-tile/per-GT subtree under device/tile#/gt#/freq0/. We read
// tile 0 / gt 0. min_freq/max_freq are the configurable soft limits (matching
// i915's gt_min/gt_max semantics); rpn_freq/rp0_freq would be the hardware
// limits. Values are in MHz like i915.
std::vector<std::uint64_t> get_intel_xe_frequencies(const std::filesystem::path& drm_path) {
  // {min, current, max}
  std::vector<std::uint64_t> freqs(3, 0);
  const std::filesystem::path freq0 = drm_path / "device/tile0/gt0/freq0";
  freqs[0] = read_freq_mhz_as_hz(freq0 / "min_freq");
  freqs[1] = read_freq_mhz_as_hz(freq0 / "cur_freq");
  freqs[2] = read_freq_mhz_as_hz(freq0 / "max_freq");
  return freqs;
}

// _____________________________________________________________________________________________________________________
// amdgpu hwmon: the true instantaneous shader clock, freq1_input (already in
// Hz), validated against freq1_label == "sclk" when the label is present.
// Returns 0 if no matching hwmon node exposes it.
std::uint64_t read_amd_hwmon_sclk_hz(const std::filesystem::path& drm_path) {
  std::error_code ec;
  const std::filesystem::path hwmon_root = drm_path / "device/hwmon";
  if (!std::filesystem::is_directory(hwmon_root, ec)) {
    return 0;
  }
  for (const auto& entry : std::filesystem::directory_iterator(hwmon_root, ec)) {
    const std::filesystem::path input = entry.path() / "freq1_input";
    if (!std::filesystem::exists(input)) {
      continue;
    }
    const std::string label = read_drm_by_path(entry.path() / "freq1_label");
    if (!label.empty() && label != "sclk") {
      continue;
    }
    return read_freq_hz(input);
  }
  return 0;
}

// _____________________________________________________________________________________________________________________
// amdgpu: min/max come from the DPM state table device/pp_dpm_sclk, e.g.
//   0: 200Mhz
//   1: 700Mhz *
//   2: 1400Mhz
// The starred line is the active (coarse) DPM level; we prefer hwmon's
// instantaneous sclk for the current value and fall back to the starred line.
std::vector<std::uint64_t> get_amd_frequencies(const std::filesystem::path& drm_path) {
  // {min, current, max}
  std::vector<std::uint64_t> freqs(3, 0);
  auto lines = read_drm_lines_by_path(drm_path / "device/pp_dpm_sclk");
  std::uint64_t min_mhz = 0;
  std::uint64_t cur_mhz = 0;
  std::uint64_t max_mhz = 0;
  bool first = true;
  for (const auto& line : lines) {
    auto colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }
    std::uint64_t mhz = 0;
    try {
      mhz = std::stoull(line.substr(colon + 1));  // stoull skips leading space, stops at "Mhz"
    } catch (...) {
      continue;
    }
    if (first || mhz < min_mhz) {
      min_mhz = mhz;
    }
    if (first || mhz > max_mhz) {
      max_mhz = mhz;
    }
    first = false;
    if (line.find('*') != std::string::npos) {
      cur_mhz = mhz;
    }
  }
  const auto mega = static_cast<std::uint64_t>(unit::SiPrefix::MEGA);
  freqs[0] = min_mhz * mega;
  freqs[2] = max_mhz * mega;
  std::uint64_t cur_hz = read_amd_hwmon_sclk_hz(drm_path);
  freqs[1] = cur_hz != 0 ? cur_hz : cur_mhz * mega;
  return freqs;
}

// _____________________________________________________________________________________________________________________
// ARM SoC GPUs (panfrost, lima, v3d, ...) expose clocks via the devfreq
// framework under device/devfreq/<node>/. Values are already in Hz, so no
// SI-prefix scaling is applied.
std::vector<std::uint64_t> get_devfreq_frequencies(const std::filesystem::path& drm_path) {
  // {min, current, max}
  std::vector<std::uint64_t> freqs(3, 0);
  const std::filesystem::path node = first_subdirectory(drm_path / "device/devfreq");
  if (node.empty()) {
    return freqs;
  }
  freqs[0] = read_freq_hz(node / "min_freq");
  freqs[1] = read_freq_hz(node / "cur_freq");
  freqs[2] = read_freq_hz(node / "max_freq");
  return freqs;
}

// _____________________________________________________________________________________________________________________
enum class GpuVendor { Intel, Amd, Nvidia, Unknown };

// _____________________________________________________________________________________________________________________
GpuVendor gpu_vendor_from_id(const std::string& vendor_id) {
  if (vendor_id == "0x8086") {
    return GpuVendor::Intel;
  }
  if (vendor_id == "0x1002") {
    return GpuVendor::Amd;
  }
  if (vendor_id == "0x10de") {
    return GpuVendor::Nvidia;
  }
  return GpuVendor::Unknown;
}

// _____________________________________________________________________________________________________________________
// The PCI vendor narrows the choice, but the driver (i.e. the sysfs layout) is
// what actually decides the interface, so the only intra-vendor probe needed is
// Intel i915 vs Xe. AMD is amdgpu. NVIDIA exposes nothing here (proprietary
// needs NVML, nouveau only via debugfs). SoC/devfreq parts have no PCI vendor
// file and are filtered out before this point; the default arm is a catch-all
// in case enumeration is later broadened to platform devices.
std::vector<std::uint64_t> get_frequencies(const std::filesystem::path& drm_path, GpuVendor vendor) {
  switch (vendor) {
    case GpuVendor::Intel:
      if (std::filesystem::exists(drm_path / "gt_cur_freq_mhz")) {
        return get_intel_i915_frequencies(drm_path);
      }
      return get_intel_xe_frequencies(drm_path);
    case GpuVendor::Amd:
      return get_amd_frequencies(drm_path);
    case GpuVendor::Nvidia:
      return std::vector<std::uint64_t>(3, 0);
    case GpuVendor::Unknown:
    default:
      return get_devfreq_frequencies(drm_path);
  }
}

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  std::vector<GPU> gpus{};
  PCIMapper pci = PCI::getMapper();

  // Enumerate the "cardN" nodes that actually exist rather than probing a
  // contiguous id range. directory_iterator yields no particular order, so we
  // collect and sort by index to keep card0 before card1, etc.
  std::vector<std::filesystem::path> card_paths;
  std::error_code ec;
  for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm", ec)) {
    const std::string name = entry.path().filename().string();
    if (name.rfind("card", 0) != 0) {
      continue;  // skip renderD*, controlD*, version, ...
    }
    if (name.find_first_not_of("0123456789", 4) != std::string::npos) {
      continue;  // skip connectors like "card0-HDMI-A-1"
    }
    card_paths.push_back(entry.path());
  }
  std::sort(card_paths.begin(), card_paths.end(), [](const auto& a, const auto& b) {
    return std::stoi(a.filename().string().substr(4)) < std::stoi(b.filename().string().substr(4));
  });

  for (const auto& path : card_paths) {
    GPU gpu;
    gpu._id = std::stoi(path.filename().string().substr(4));
    gpu._vendor_id = read_drm_by_path(path / "device/vendor");
    gpu._device_id = read_drm_by_path(path / "device/device");
    if (gpu._vendor_id.empty() || gpu._device_id.empty()) {
      continue;
    }
    const PCIVendor& vendor = pci[gpu._vendor_id];
    const PCIDevice& device = vendor[gpu._device_id];
    gpu._vendor = vendor.vendor_name;
    gpu._name = device.device_name;
    auto frequencies = get_frequencies(path, gpu_vendor_from_id(gpu._vendor_id));
    gpu._frequency_hz = frequencies[2];
    gpus.push_back(std::move(gpu));
  }
#ifdef USE_OCL
  auto cl_gpus = opencl_::DeviceManager::get_list<opencl_::Filter::GPU>();
  if (!gpus.empty()) {
    for (auto& gpu : gpus) {
      for (auto* cl_gpu : cl_gpus) {
        if (cl_gpu->name().find(gpu._device_id)) {
          gpu._driverVersion = cl_gpu->driver_version();
          gpu._frequency_hz = static_cast<int64_t>(cl_gpu->clock_frequency_MHz()) * 1000 * 1000;
          gpu._num_cores = static_cast<int>(cl_gpu->cores());
          gpu._dedicated_memory_Bytes = static_cast<int64_t>(cl_gpu->memory_Bytes());
        }
      }
    }
  } else {
    std::regex pciid_regex("0x[0-9a-fA-F]{4}");
    for (auto* cl_gpu : cl_gpus) {
      GPU gpu;
      std::stringstream ss;
      ss << std::hex << cl_gpu->vendor_id();
      gpu._vendor_id = ss.str();
      gpu._id = cl_gpu->get_id();
      gpu._vendor = cl_gpu->vendor();
      gpu._name = cl_gpu->name();
      gpu._frequency_hz = cl_gpu->clock_frequency_MHz() * 1000 * 1000;
      gpu._num_cores = cl_gpu->cores();
      if (cl_gpu->unified_memory()) {
        gpu._shared_memory_Bytes = cl_gpu->memory_Bytes();
      } else {
        gpu._dedicated_memory_Bytes = cl_gpu->memory_Bytes();
      }
      gpu._driverVersion = cl_gpu->driver_version();
      const PCIVendor& vendor = pci[gpu._vendor_id];
      if (vendor.vendor_name != "invalid") {
        gpu._vendor = vendor.vendor_name;
        std::smatch m;
        if (std::regex_search(gpu.name(), m, pciid_regex)) {
          const PCIDevice& device = vendor[m.str()];
          if (device.device_name != "invalid") {
            gpu._device_id = device.device_id;
            gpu._name = device.device_name;
          }
        }
      }

      gpus.push_back(std::move(gpu));
    }
  }
#endif  // USE_OCL
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX