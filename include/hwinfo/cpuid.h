// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#if defined(__x86_64__) || defined(__x86_64) || defined(_M_IX86)

#ifndef HWINFO_CPUID_H_
#define HWINFO_CPUID_H_

#pragma once

#include <string>

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
void cpuid(unsigned func_id, unsigned sub_func_id, uint32_t regs[4]) {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
  __cpuidex((int*) regs, static_cast<int>(func_id), static_cast<int>(sub_func_id));
#else
  asm volatile ("cpuid" :"=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func_id), "c" (sub_func_id));
#endif
}

}  // namespace hwinfo::cpuid

#endif  //HWINFO_CPUID_H_
#endif  // check x86 arch