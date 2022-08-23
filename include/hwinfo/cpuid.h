// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#ifndef HWINFO_CPUID_H_
#define HWINFO_CPUID_H_

namespace hwinfo {

#define MAX_INTEL_TOP_LVL 4

class CPUID {
 public:
  explicit CPUID(unsigned funcId, unsigned subFuncId) {
#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
    __cpuidex((int*) _regs, (int) funcId, (int) subFuncId);
#else
    asm volatile ("cpuid" :"=a" (_regs[0]), "=b" (_regs[1]), "=c" (_regs[2]), "=d" (_regs[3]) : "a" (funcId), "c" (subFuncId));
#endif
  }

  [[nodiscard]] const uint32_t &EAX() const { return _regs[0]; }
  [[nodiscard]] const uint32_t &EBX() const { return _regs[1]; }
  [[nodiscard]] const uint32_t &ECX() const {return _regs[2]; }
  [[nodiscard]] const uint32_t &EDX() const {return _regs[3]; }

 private:
  uint32_t _regs[4]{};
};

}  // hwinfo

#endif //HWINFO_CPUID_H_
