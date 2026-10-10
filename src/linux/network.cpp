// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_UNIX

#include <arpa/inet.h>
#include <hwinfo/network.h>
#include <ifaddrs.h>
#include <net/if.h>

#include <algorithm>
#include <cerrno>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "internal/file.h"

namespace hwinfo {

result<std::vector<NetworkInterface>> network_interfaces() {
  ifaddrs* raw = nullptr;
  if (getifaddrs(&raw) == -1) {
    return std::unexpected(error{std::error_code(errno, std::generic_category()), "getifaddrs"});
  }
  const std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> addresses(raw, &freeifaddrs);

  std::vector<NetworkInterface> result;
  for (const ifaddrs* ifa = addresses.get(); ifa != nullptr; ifa = ifa->ifa_next) {
    const std::string_view name = ifa->ifa_name;
    auto nic = std::ranges::find(result, name, &NetworkInterface::name);
    if (nic == result.end()) {
      const std::filesystem::path sysfs = std::filesystem::path("/sys/class/net") / name;
      result.push_back(NetworkInterface{
          .index = if_nametoindex(ifa->ifa_name),
          .name = std::string(name),
          .description = std::nullopt,
          .mac = internal::read_attribute(sysfs / "address"),
          .ipv4 = {},
          .ipv6 = {},
          .is_up = (ifa->ifa_flags & IFF_UP) != 0,
          .is_loopback = (ifa->ifa_flags & IFF_LOOPBACK) != 0,
      });
      nic = std::prev(result.end());
    }
    if (ifa->ifa_addr == nullptr) {
      continue;
    }
    if (ifa->ifa_addr->sa_family == AF_INET) {
      char ip[INET_ADDRSTRLEN]{};
      inet_ntop(AF_INET, &reinterpret_cast<const sockaddr_in*>(ifa->ifa_addr)->sin_addr, ip, sizeof(ip));
      nic->ipv4.emplace_back(ip);
    } else if (ifa->ifa_addr->sa_family == AF_INET6) {
      char ip[INET6_ADDRSTRLEN]{};
      inet_ntop(AF_INET6, &reinterpret_cast<const sockaddr_in6*>(ifa->ifa_addr)->sin6_addr, ip, sizeof(ip));
      nic->ipv6.emplace_back(ip);
    }
  }
  std::ranges::sort(result, {}, &NetworkInterface::index);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_UNIX
