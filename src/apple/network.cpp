// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <arpa/inet.h>
#include <hwinfo/network.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <algorithm>
#include <cerrno>
#include <format>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hwinfo {

namespace {

// The link layer address of an AF_LINK entry, "aa:bb:cc:dd:ee:ff".
// std::nullopt for interfaces without one (lo0, utun, ...).
std::optional<std::string> mac_address(const sockaddr* address) {
  const auto* link = reinterpret_cast<const sockaddr_dl*>(address);
  if (link->sdl_alen != 6) {
    return std::nullopt;
  }
  const auto* bytes = reinterpret_cast<const unsigned char*>(link->sdl_data + link->sdl_nlen);
  if (std::all_of(bytes, bytes + link->sdl_alen, [](unsigned char b) { return b == 0; })) {
    return std::nullopt;
  }
  return std::format("{:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}", bytes[0], bytes[1], bytes[2], bytes[3], bytes[4],
                     bytes[5]);
}

}  // namespace

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
      result.push_back(NetworkInterface{
          .index = if_nametoindex(ifa->ifa_name),
          .name = std::string(name),
          .description = std::nullopt,
          .mac = std::nullopt,
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
    if (ifa->ifa_addr->sa_family == AF_LINK) {
      if (!nic->mac) {
        nic->mac = mac_address(ifa->ifa_addr);
      }
    } else if (ifa->ifa_addr->sa_family == AF_INET) {
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

#endif  // HWINFO_APPLE
