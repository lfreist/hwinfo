// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// CPUID on x86. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_X86

#include <cstdint>
#include <cstring>
#include <string>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

#include "strings.h"

namespace hwinfo::internal {

struct CpuidRegisters {
  std::uint32_t eax = 0;
  std::uint32_t ebx = 0;
  std::uint32_t ecx = 0;
  std::uint32_t edx = 0;
};

inline CpuidRegisters cpuid(std::uint32_t leaf, std::uint32_t subleaf = 0) {
#if defined(_MSC_VER)
  int r[4]{};
  __cpuidex(r, static_cast<int>(leaf), static_cast<int>(subleaf));
  return {static_cast<std::uint32_t>(r[0]), static_cast<std::uint32_t>(r[1]), static_cast<std::uint32_t>(r[2]),
          static_cast<std::uint32_t>(r[3])};
#else
  CpuidRegisters r;
  __cpuid_count(leaf, subleaf, r.eax, r.ebx, r.ecx, r.edx);
  return r;
#endif
}

// 12 character vendor signature in EBX, ECX, EDX (hypervisor leaves), without padding.
inline std::string cpuid_vendor(const CpuidRegisters& r) {
  char vendor[12];
  std::memcpy(vendor, &r.ebx, 4);
  std::memcpy(vendor + 4, &r.ecx, 4);
  std::memcpy(vendor + 8, &r.edx, 4);
  std::string_view s(vendor, sizeof(vendor));
  s = s.substr(0, s.find('\0'));
  return std::string(trim(s));
}

}  // namespace hwinfo::internal

#endif  // HWINFO_X86
