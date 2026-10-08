// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Internal Mach helpers (macOS). Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <hwinfo/error.h>
#include <mach/mach.h>
#include <mach/mach_error.h>

#include <format>
#include <string_view>

namespace hwinfo::internal::apple {

// Error for a failed kern_return_t / IOReturn, e.g. kern_error(kr, "host_processor_info").
inline error kern_error(kern_return_t code, std::string_view what) {
  return error{errc::platform_error,
               std::format("{}: {} ({:#x})", what, mach_error_string(code), static_cast<unsigned>(code))};
}

class host_port {
 public:
  host_port() noexcept : _port(mach_host_self()) {}
  host_port(const host_port&) = delete;
  host_port& operator=(const host_port&) = delete;
  ~host_port() { mach_port_deallocate(mach_task_self(), _port); }

  [[nodiscard]] host_t get() const noexcept { return _port; }

 private:
  host_t _port;
};

}  // namespace hwinfo::internal::apple

#endif  // HWINFO_APPLE
