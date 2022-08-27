// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <string>
#ifndef _MSC_VER
#include <cpuid.h>
#endif

#include "platform.h"

#ifdef HWINFO_X86

#define MAX_INTEL_TOP_LVL 4

#define SSE_POS 0x02000000
#define SSE2_POS 0x04000000
#define SSE3_POS 0x00000001
#define SSE41_POS 0x00080000
#define SSE42_POS 0x00100000
#define AVX_POS 0x10000000
#define AVX2_POS 0x00000020
#define LVL_NUM 0x000000ff
#define LVL_TYPE 0x0000ff00
#define LVL_CORES 0x0000ffff

namespace hwinfo::cpuid {

/**
 * cpuid wrapper for Linux, MacOS and Windows
 * @param func_id
 * @param sub_func_id
 * @param regs
 */
void cpuid(uint32_t func_id, uint32_t sub_func_id, uint32_t regs[4]) {
#ifdef _MSC_VER
    CpuIdEx(reinterpret_cast<int*>(regs), static_cast<int>(func_id), static_cast<int>(sub_func_id));
#elif defined(__GNUC__) || defined (__clang__)
    __get_cpuid_count(func_id, sub_func_id, &regs[0], &regs[1], &regs[2], &regs[3]);
#elif __CYGWIN__
    cpuid(&regs[0], &regs[1], &regs[2], &regs[3], func_id, sub_func_id);
#endif
}

}  // namespace hwinfo::cpuid

#endif
