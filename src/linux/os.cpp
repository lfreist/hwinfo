// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <hwinfo/os.h>
#include <sys/utsname.h>

#include <cerrno>
#include <string>
#include <string_view>

#include "internal/file.h"
#include "internal/procfs.h"

namespace hwinfo {

namespace {

OsFamily family_of(std::string_view sysname) {
  if (sysname == "Linux") {
    return OsFamily::linux_;
  }
  if (sysname.ends_with("BSD")) {  // FreeBSD, OpenBSD, NetBSD, DragonFlyBSD
    return OsFamily::bsd;
  }
  return OsFamily::unknown;
}

}  // namespace

result<Os> os() {
  utsname info{};
  if (uname(&info) != 0) {
    return std::unexpected(error{std::error_code(errno, std::generic_category()), "uname"});
  }
  Os os{
      .family = family_of(info.sysname),
      .name = info.sysname,
      .marketing_name = {},
      .version = {},
      .kernel = info.release,
      .architecture = info.machine,
      .bits = std::string_view(info.machine).contains("64") || std::string_view(info.machine) == "s390x" ? 64u : 32u,
  };

  auto release = internal::read_file("/etc/os-release");
  if (!release) {
    release = internal::read_file("/usr/lib/os-release");
  }
  if (release) {
    const auto values = internal::procfs::parse_os_release(*release);
    if (const auto it = values.find("NAME"); it != values.end()) {
      os.name = it->second;
    }
    if (const auto it = values.find("VERSION"); it != values.end()) {
      os.version = it->second;
    } else if (const auto id = values.find("VERSION_ID"); id != values.end()) {
      os.version = id->second;
    } else if (const auto build = values.find("BUILD_ID"); build != values.end()) {
      os.version = build->second;  // rolling releases, e.g. Arch Linux
    }
    os.marketing_name = internal::procfs::os_release_codename(values);
  }
  return os;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
