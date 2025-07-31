// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "hwinfo/platform.h"

#ifdef HWINFO_APPLE

#include <regex>
#include <string>
#include <vector>

#include "hwinfo/gpu.h"
#import <Metal/Metal.h>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  NSArray<id<MTLDevice>>* devices = MTLCopyAllDevices();

  std::vector<GPU> gpus{};
  for (id<MTLDevice> device in devices) {
    GPU gpu;
    gpu._name = std::string{[device.name UTF8String], device.name.length};
    if (@available(macos 14.0, ios 16.0, *)) {
      gpu._vendor = "Apple " + std::string{[device.architecture.name UTF8String], device.architecture.name.length};
    } else {
      gpu._vendor = "?";
    }
    gpu._memory_Bytes = device.recommendedMaxWorkingSetSize;
    gpu._vendor_id = "?";
    gpu._device_id = device.registryID;

    // there is not cache_size and max_frequency in MTLDevice
    gpus.push_back(std::move(gpu));
  }
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
