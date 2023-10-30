// Copyright MRsoymilk
// Author MRsoymilk <codermrsoymilk@gmail.com>

#pragma once

#include <hwinfo/platform.h>

#include <string>
#include <vector>

namespace hwinfo {

class Network {
 public:
  friend std::vector<Network> getAllNetworks();

public:
  ~Network() = default;

  HWI_NODISCARD const std::string& interfaceIndex() const;
  HWI_NODISCARD const std::string& mac() const;
  HWI_NODISCARD const std::string& ip4() const;
  HWI_NODISCARD const std::string& ip6() const;
  HWI_NODISCARD const std::string& description() const;

 private:
  Network() = default; 
  std::string _interfaceIndex{};
  std::string _mac{};
  std::string _ipv4{}, _ipv6{};
  std::string _description{};
};

std::vector<Network> getAllNetworks();
}  // namespace hwinfo
