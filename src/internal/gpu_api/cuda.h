// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// The subset of the CUDA driver API used by hwinfo, for loading libcuda / nvcuda.dll at runtime.
// Mirrors cuda.h of CUDA 12 (the used functions and attributes are stable since CUDA 4).
// Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#include <cstddef>

#ifdef HWINFO_WINDOWS
#define HWINFO_CUDA_CALL __stdcall
#else
#define HWINFO_CUDA_CALL
#endif

namespace hwinfo::internal::cuda {

using CUresult = int;
using CUdevice = int;
using CUdevice_attribute = int;

constexpr CUresult CUDA_SUCCESS = 0;

constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_CLOCK_RATE = 13;  // kHz
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT = 16;
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_INTEGRATED = 18;
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_PCI_BUS_ID = 33;
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_PCI_DEVICE_ID = 34;            // the device number on the bus
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE = 36;        // kHz
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH = 37;  // bits
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_L2_CACHE_SIZE = 38;            // bytes
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_PCI_DOMAIN_ID = 50;
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MAJOR = 75;
constexpr CUdevice_attribute CU_DEVICE_ATTRIBUTE_COMPUTE_CAPABILITY_MINOR = 76;

struct CUuuid {
  char bytes[16];
};

using cuInit_t = CUresult(HWINFO_CUDA_CALL*)(unsigned int flags);
using cuDriverGetVersion_t = CUresult(HWINFO_CUDA_CALL*)(int* version);  // 1000 * major + 10 * minor
using cuDeviceGetCount_t = CUresult(HWINFO_CUDA_CALL*)(int* count);
using cuDeviceGet_t = CUresult(HWINFO_CUDA_CALL*)(CUdevice* device, int ordinal);
using cuDeviceGetName_t = CUresult(HWINFO_CUDA_CALL*)(char* name, int length, CUdevice device);
using cuDeviceGetAttribute_t = CUresult(HWINFO_CUDA_CALL*)(int* value, CUdevice_attribute attribute, CUdevice device);
using cuDeviceGetUuid_t = CUresult(HWINFO_CUDA_CALL*)(CUuuid* uuid, CUdevice device);
using cuDeviceTotalMem_v2_t = CUresult(HWINFO_CUDA_CALL*)(std::size_t* bytes, CUdevice device);
using cuDeviceGetLuid_t = CUresult(HWINFO_CUDA_CALL*)(char* luid, unsigned int* node_mask, CUdevice device);

static_assert(sizeof(CUuuid) == 16);

}  // namespace hwinfo::internal::cuda

#undef HWINFO_CUDA_CALL
