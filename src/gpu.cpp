// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/gpu.h>

#include <string>

#ifdef USE_OCL
#define CL_HPP_ENABLE_EXCEPTIONS
#define CL_HPP_TARGET_OPENCL_VERSION 200
#include <CL/opencl.hpp>
#endif

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& GPU::vendor() const { return _vendor; }

// _____________________________________________________________________________________________________________________
const std::string& GPU::name() const { return _name; }

// _____________________________________________________________________________________________________________________
const std::string& GPU::driverVersion() const { return _driverVersion; }

// _____________________________________________________________________________________________________________________
int GPU::id() const { return _id; }

// _____________________________________________________________________________________________________________________
int64_t GPU::memory_Bytes() const { return _memory_Bytes; }

// _____________________________________________________________________________________________________________________
int64_t GPU::frequency_MHz() const { return _frequency_MHz; }

// _____________________________________________________________________________________________________________________
int GPU::num_cores() const { return _num_cores; }

#ifdef USE_OCL

std::vector<GPU_CL> get_cpu_cl_data() {
  std::vector<GPU_CL> gpus;
  std::vector<cl::Platform> cl_platforms;
  auto res = cl::Platform::get(&cl_platforms);
  if (res != CL_SUCCESS) {
    return {};
  }
  int id = 0;
  for (auto& clp : cl_platforms) {
    std::vector<cl::Device> cl_devices;
    clp.getDevices(CL_DEVICE_TYPE_GPU, &cl_devices);
    for (auto& cld : cl_devices) {
      GPU_CL gpu;
      cl::Context cl_context(cld);
      gpu.id = id;
      gpu.vendor = cld.getInfo<CL_DEVICE_VENDOR>();
      gpu.name = cld.getInfo<CL_DEVICE_NAME>();
      gpu.driver_version = cld.getInfo<CL_DRIVER_VERSION>();
      gpu.memory_Bytes = cld.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>();
      gpu.frequency_MHz = cld.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>();
      gpu.num_cores = cld.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>();
      gpus.push_back(gpu);
    }
  }
  return gpus;
}

#endif

}  // namespace hwinfo