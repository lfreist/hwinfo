// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/monitoring.h>
#include <mach/mach.h>

#include <algorithm>
#include <cstdint>

#include "internal/apple_mach.h"
#include "internal/sysctl.h"

namespace hwinfo {

result<MemoryUsage> memory_usage() {
  const auto total = internal::sysctl_value<std::uint64_t>("hw.memsize");
  if (!total) {
    return std::unexpected(total.error());
  }

  const internal::apple::host_port host;
  vm_statistics64_data_t vm{};
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
  const kern_return_t code =
      host_statistics64(host.get(), HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&vm), &count);
  if (code != KERN_SUCCESS) {
    return std::unexpected(internal::apple::kern_error(code, "host_statistics64"));
  }

  vm_size_t page_size = 0;
  if (const kern_return_t page_code = host_page_size(host.get(), &page_size); page_code != KERN_SUCCESS) {
    return std::unexpected(internal::apple::kern_error(page_code, "host_page_size"));
  }
  // free_count includes the speculative pages (see vm_stat)
  const std::uint64_t free_pages = vm.free_count - std::min(vm.free_count, vm.speculative_count);
  const std::uint64_t available_pages =
      std::uint64_t{vm.free_count} + std::uint64_t{vm.inactive_count} + std::uint64_t{vm.purgeable_count};
  return MemoryUsage{
      .total = Bytes{*total},
      .free = Bytes{std::min(free_pages * std::uint64_t{page_size}, *total)},
      .available = Bytes{std::min(available_pages * std::uint64_t{page_size}, *total)},
  };
}

}  // namespace hwinfo

#endif  // HWINFO_APPLE
