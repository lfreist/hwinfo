// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// The subset of the OpenCL API used by hwinfo, for loading the ICD loader at runtime.
// Mirrors CL/cl.h and CL/cl_ext.h of the Khronos OpenCL-Headers (ABI stable since OpenCL 1.0) and the vendor
// extensions cl_khr_pci_bus_info, cl_khr_device_uuid, cl_nv_device_attribute_query, cl_amd_device_attribute_query and
// cl_intel_device_attribute_query.
// Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#include <cstddef>
#include <cstdint>

#ifdef HWINFO_WINDOWS
#define HWINFO_CL_CALL __stdcall
#else
#define HWINFO_CL_CALL
#endif

namespace hwinfo::internal::cl {

using cl_int = std::int32_t;
using cl_uint = std::uint32_t;
using cl_ulong = std::uint64_t;
using cl_bool = cl_uint;
using cl_char = std::int8_t;
using cl_device_type = cl_ulong;
using cl_platform_info = cl_uint;
using cl_device_info = cl_uint;
using cl_platform_id = struct _cl_platform_id*;
using cl_device_id = struct _cl_device_id*;

constexpr cl_int CL_SUCCESS = 0;
constexpr cl_device_type CL_DEVICE_TYPE_GPU = 1 << 2;

constexpr cl_platform_info CL_PLATFORM_VERSION = 0x0901;
constexpr cl_platform_info CL_PLATFORM_NAME = 0x0902;

constexpr cl_device_info CL_DEVICE_VENDOR_ID = 0x1001;
constexpr cl_device_info CL_DEVICE_MAX_COMPUTE_UNITS = 0x1002;
constexpr cl_device_info CL_DEVICE_MAX_CLOCK_FREQUENCY = 0x100C;  // MHz
constexpr cl_device_info CL_DEVICE_GLOBAL_MEM_CACHE_SIZE = 0x101E;
constexpr cl_device_info CL_DEVICE_GLOBAL_MEM_SIZE = 0x101F;
constexpr cl_device_info CL_DEVICE_NAME = 0x102B;
constexpr cl_device_info CL_DRIVER_VERSION = 0x102D;
constexpr cl_device_info CL_DEVICE_VERSION = 0x102F;  // "OpenCL <major>.<minor> <vendor specific>"
constexpr cl_device_info CL_DEVICE_EXTENSIONS = 0x1030;
constexpr cl_device_info CL_DEVICE_HOST_UNIFIED_MEMORY = 0x1035;  // deprecated in 2.0, still answered

// cl_khr_device_uuid
constexpr cl_device_info CL_DEVICE_UUID_KHR = 0x106A;
constexpr cl_device_info CL_DEVICE_LUID_VALID_KHR = 0x106C;
constexpr cl_device_info CL_DEVICE_LUID_KHR = 0x106D;
constexpr std::size_t CL_UUID_SIZE_KHR = 16;
constexpr std::size_t CL_LUID_SIZE_KHR = 8;

// cl_khr_pci_bus_info
constexpr cl_device_info CL_DEVICE_PCI_BUS_INFO_KHR = 0x410F;
struct cl_device_pci_bus_info_khr {
  cl_uint pci_domain;
  cl_uint pci_bus;
  cl_uint pci_device;
  cl_uint pci_function;
};

// cl_nv_device_attribute_query
constexpr cl_device_info CL_DEVICE_COMPUTE_CAPABILITY_MAJOR_NV = 0x4000;
constexpr cl_device_info CL_DEVICE_COMPUTE_CAPABILITY_MINOR_NV = 0x4001;
constexpr cl_device_info CL_DEVICE_INTEGRATED_MEMORY_NV = 0x4006;
constexpr cl_device_info CL_DEVICE_PCI_BUS_ID_NV = 0x4008;
constexpr cl_device_info CL_DEVICE_PCI_SLOT_ID_NV = 0x4009;  // device << 3 | function
constexpr cl_device_info CL_DEVICE_PCI_DOMAIN_ID_NV = 0x400A;

// cl_amd_device_attribute_query
constexpr cl_device_info CL_DEVICE_TOPOLOGY_AMD = 0x4037;
constexpr cl_device_info CL_DEVICE_BOARD_NAME_AMD = 0x4038;
constexpr cl_device_info CL_DEVICE_SIMD_PER_COMPUTE_UNIT_AMD = 0x4040;
constexpr cl_device_info CL_DEVICE_GFXIP_MAJOR_AMD = 0x404A;
constexpr cl_device_info CL_DEVICE_GFXIP_MINOR_AMD = 0x404B;
constexpr cl_uint CL_DEVICE_TOPOLOGY_TYPE_PCIE_AMD = 1;
union cl_device_topology_amd {
  struct {
    cl_uint type;
    cl_uint data[5];
  } raw;
  struct {
    cl_uint type;
    cl_char unused[17];
    cl_char bus;
    cl_char device;
    cl_char function;
  } pcie;
};

// cl_intel_device_attribute_query
constexpr cl_device_info CL_DEVICE_IP_VERSION_INTEL = 0x4250;
constexpr cl_device_info CL_DEVICE_NUM_SLICES_INTEL = 0x4252;
constexpr cl_device_info CL_DEVICE_NUM_SUB_SLICES_PER_SLICE_INTEL = 0x4253;
constexpr cl_device_info CL_DEVICE_NUM_EUS_PER_SUB_SLICE_INTEL = 0x4254;

using clGetPlatformIDs_t = cl_int(HWINFO_CL_CALL*)(cl_uint num_entries, cl_platform_id* platforms,
                                                   cl_uint* num_platforms);
using clGetPlatformInfo_t = cl_int(HWINFO_CL_CALL*)(cl_platform_id platform, cl_platform_info param_name,
                                                    std::size_t param_value_size, void* param_value,
                                                    std::size_t* param_value_size_ret);
using clGetDeviceIDs_t = cl_int(HWINFO_CL_CALL*)(cl_platform_id platform, cl_device_type device_type,
                                                 cl_uint num_entries, cl_device_id* devices, cl_uint* num_devices);
using clGetDeviceInfo_t = cl_int(HWINFO_CL_CALL*)(cl_device_id device, cl_device_info param_name,
                                                  std::size_t param_value_size, void* param_value,
                                                  std::size_t* param_value_size_ret);

static_assert(sizeof(cl_device_pci_bus_info_khr) == 16);
static_assert(sizeof(cl_device_topology_amd) == 24);

}  // namespace hwinfo::internal::cl

#undef HWINFO_CL_CALL
