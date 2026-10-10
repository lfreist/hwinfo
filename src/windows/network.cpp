// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
// clang-format on
#include <hwinfo/network.h>

#include <algorithm>
#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <vector>

#include "internal/strings.h"
#include "internal/windows_error.h"
#include "internal/windows_strings.h"

#ifdef _MSC_VER
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")
#endif

namespace hwinfo {

namespace {

// "aa:bb:cc:dd:ee:ff"; std::nullopt for interfaces without a hardware address (loopback, tunnels).
std::optional<std::string> mac_address(const IP_ADAPTER_ADDRESSES& adapter) {
  if (adapter.PhysicalAddressLength == 0) {
    return std::nullopt;
  }
  std::string mac;
  for (ULONG i = 0; i < adapter.PhysicalAddressLength; ++i) {
    mac += std::format("{}{:02x}", i == 0 ? "" : ":", adapter.PhysicalAddress[i]);
  }
  return mac;
}

std::optional<std::string> ip_address(const SOCKADDR* address) {
  char buffer[INET6_ADDRSTRLEN]{};
  if (address->sa_family == AF_INET) {
    const auto* ipv4 = reinterpret_cast<const sockaddr_in*>(address);
    return inet_ntop(AF_INET, &ipv4->sin_addr, buffer, sizeof(buffer)) ? std::optional<std::string>(buffer)
                                                                       : std::nullopt;
  }
  if (address->sa_family == AF_INET6) {
    const auto* ipv6 = reinterpret_cast<const sockaddr_in6*>(address);
    return inet_ntop(AF_INET6, &ipv6->sin6_addr, buffer, sizeof(buffer)) ? std::optional<std::string>(buffer)
                                                                         : std::nullopt;
  }
  return std::nullopt;
}

result<std::vector<std::byte>> adapter_addresses() {
  constexpr ULONG flags = GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER;
  ULONG size = 16 * 1024;
  for (int attempt = 0; attempt < 4; ++attempt) {
    std::vector<std::byte> buffer(size);
    const ULONG status =
        GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data()), &size);
    if (status == ERROR_SUCCESS) {
      return buffer;
    }
    if (status == ERROR_NO_DATA) {
      return std::vector<std::byte>{};  // no network interfaces
    }
    if (status != ERROR_BUFFER_OVERFLOW) {
      return std::unexpected(internal::win32_error(status, "GetAdaptersAddresses"));
    }
  }
  return std::unexpected(internal::win32_error(ERROR_BUFFER_OVERFLOW, "GetAdaptersAddresses"));
}

}  // namespace

result<std::vector<NetworkInterface>> network_interfaces() {
  const auto buffer = adapter_addresses();
  if (!buffer) {
    return std::unexpected(buffer.error());
  }
  std::vector<NetworkInterface> result;
  if (buffer->empty()) {
    return result;
  }
  for (auto* adapter = reinterpret_cast<const IP_ADAPTER_ADDRESSES*>(buffer->data()); adapter != nullptr;
       adapter = adapter->Next) {
    NetworkInterface nic{
        .index = adapter->IfIndex != 0 ? adapter->IfIndex : adapter->Ipv6IfIndex,
        .name = internal::to_utf8(adapter->FriendlyName),  // e.g. "Ethernet", "Wi-Fi"
        .description = internal::non_empty(internal::to_utf8(adapter->Description)),
        .mac = mac_address(*adapter),
        .ipv4 = {},
        .ipv6 = {},
        .is_up = adapter->OperStatus == IfOperStatusUp,
        .is_loopback = adapter->IfType == IF_TYPE_SOFTWARE_LOOPBACK,
    };
    for (auto* unicast = adapter->FirstUnicastAddress; unicast != nullptr; unicast = unicast->Next) {
      const SOCKADDR* address = unicast->Address.lpSockaddr;
      if (address == nullptr) {
        continue;
      }
      if (auto ip = ip_address(address)) {
        (address->sa_family == AF_INET ? nic.ipv4 : nic.ipv6).push_back(std::move(*ip));
      }
    }
    result.push_back(std::move(nic));
  }
  std::ranges::sort(result, {}, &NetworkInterface::index);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
