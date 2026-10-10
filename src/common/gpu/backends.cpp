// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <vector>

#include "internal/gpu_backend.h"

namespace hwinfo::internal::gpu {

std::vector<BackendResult> query_backends([[maybe_unused]] const GpuQuery& query) {
  std::vector<BackendResult> results;
#ifndef HWINFO_GPU_NO_BACKENDS
  if (query.nvml) {
    results.push_back(nvml_devices());
  }
  if (query.cuda) {
    results.push_back(cuda_devices());
  }
  if (query.level_zero) {
    results.push_back(level_zero_devices());
  }
  if (query.opencl) {
    results.push_back(opencl_devices());
  }
#endif
  return results;
}

}  // namespace hwinfo::internal::gpu
