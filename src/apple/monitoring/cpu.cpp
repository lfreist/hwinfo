// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/monitoring.h>
#include <mach/mach.h>

#include <cstdint>
#include <vector>

#include "internal/apple_mach.h"
#include "internal/cpu_ticks.h"
#include "internal/sysctl.h"

namespace hwinfo {

result<std::vector<detail::CpuTicks>> internal::read_cpu_ticks() {
  const apple::host_port host;
  natural_t cpu_count = 0;
  processor_info_array_t info = nullptr;
  mach_msg_type_number_t info_count = 0;
  const kern_return_t code = host_processor_info(host.get(), PROCESSOR_CPU_LOAD_INFO, &cpu_count, &info, &info_count);
  if (code != KERN_SUCCESS) {
    return std::unexpected(apple::kern_error(code, "host_processor_info"));
  }
  const auto* load = reinterpret_cast<const processor_cpu_load_info_data_t*>(info);

  std::vector<detail::CpuTicks> ticks(cpu_count + 1);
  for (natural_t i = 0; i < cpu_count; ++i) {
    const auto& cpu = load[i].cpu_ticks;
    const std::uint64_t busy =
        std::uint64_t{cpu[CPU_STATE_USER]} + std::uint64_t{cpu[CPU_STATE_SYSTEM]} + std::uint64_t{cpu[CPU_STATE_NICE]};
    const std::uint64_t total = busy + std::uint64_t{cpu[CPU_STATE_IDLE]};
    ticks[i + 1] = {.busy = busy, .total = total};
    ticks[0].busy += busy;
    ticks[0].total += total;
  }
  // the array is allocated in our address space by the kernel; info_count is given in integer_t
  vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(info),
                static_cast<vm_size_t>(info_count) * sizeof(integer_t));
  return ticks;
}

result<std::vector<Hertz>> cpu_frequencies() {
  const auto logical = internal::sysctl_value<std::uint32_t>("hw.logicalcpu");
  if (!logical) {
    return std::unexpected(logical.error());
  }
  const auto hz = internal::sysctl_number<std::uint64_t>("hw.cpufrequency");
  if (!hz || *hz == 0) {
    return std::unexpected(error{errc::not_supported, "sysctl hw.cpufrequency is not available"});
  }
  return std::vector<Hertz>(*logical, Hertz{*hz});
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
