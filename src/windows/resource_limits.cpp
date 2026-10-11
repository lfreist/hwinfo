// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <hwinfo/resource_limits.h>
#include <windows.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include "internal/windows_processors.h"

namespace hwinfo {

namespace {

void append_processors(std::vector<std::uint32_t>& numbers, WORD group, std::uint64_t mask) {
  GROUP_AFFINITY affinity{};
  affinity.Mask = static_cast<KAFFINITY>(mask);
  affinity.Group = group;
  const auto processors = internal::processors(affinity);
  numbers.insert(numbers.end(), processors.begin(), processors.end());
}

std::vector<std::uint32_t> allowed_cpus() {
  USHORT count = 0;
  GetProcessGroupAffinity(GetCurrentProcess(), &count, nullptr);  // fails with ERROR_INSUFFICIENT_BUFFER
  std::vector<USHORT> groups(count);
  if (count == 0 || !GetProcessGroupAffinity(GetCurrentProcess(), &count, groups.data())) {
    return {};
  }
  groups.resize(count);

  std::vector<std::uint32_t> numbers;
  DWORD_PTR process_mask = 0;
  DWORD_PTR system_mask = 0;
  // the process affinity mask is only defined for processes within a single group
  if (groups.size() == 1 && GetProcessAffinityMask(GetCurrentProcess(), &process_mask, &system_mask)) {
    append_processors(numbers, groups.front(), static_cast<std::uint64_t>(process_mask));
    return numbers;
  }
  for (const USHORT group : groups) {
    const DWORD active = GetActiveProcessorCount(group);
    append_processors(numbers, group, active >= 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << active) - 1);
  }
  std::ranges::sort(numbers);
  return numbers;
}

}  // namespace

result<ResourceLimits> resource_limits() {
  ResourceLimits limits{.allowed_cpus = allowed_cpus()};

  BOOL in_job = FALSE;
  if (!IsProcessInJob(GetCurrentProcess(), nullptr, &in_job) || !in_job) {
    return limits;
  }

  JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpu{};
  if (QueryInformationJobObject(nullptr, JobObjectCpuRateControlInformation, &cpu, sizeof(cpu), nullptr) &&
      (cpu.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_ENABLE) != 0) {
    std::optional<DWORD> rate;
    if ((cpu.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE) != 0) {
      rate = cpu.CpuRate >> 16;
    } else if ((cpu.ControlFlags & JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP) != 0) {
      rate = cpu.CpuRate;
    }
    if (rate) {
      limits.cpu_quota = static_cast<double>(*rate) / 10000.0 * GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    }
  }
  JOBOBJECT_EXTENDED_LIMIT_INFORMATION extended{};
  if (QueryInformationJobObject(nullptr, JobObjectExtendedLimitInformation, &extended, sizeof(extended), nullptr)) {
    const DWORD flags = extended.BasicLimitInformation.LimitFlags;
    if ((flags & JOB_OBJECT_LIMIT_JOB_MEMORY) != 0) {
      limits.memory = Bytes{extended.JobMemoryLimit};
    }
    if ((flags & JOB_OBJECT_LIMIT_PROCESS_MEMORY) != 0) {
      const Bytes process{extended.ProcessMemoryLimit};
      limits.memory = limits.memory ? std::min(*limits.memory, process) : process;
    }
  }
  return limits;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
