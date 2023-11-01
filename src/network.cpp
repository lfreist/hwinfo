// Copyright MRsoymilk
// Author MRsoymilk <codermrsoymilk@gmail.com>

#include "hwinfo/network.h"

#include <string>

namespace hwinfo {

// _____________________________________________________________________________________________________________________
const std::string& Network::interfaceIndex() const { return _interfaceIndex;}

// _____________________________________________________________________________________________________________________
const std::string& Network::mac() const { return _mac; }

// _____________________________________________________________________________________________________________________
const std::string& Network::ip4() const { return _ipv4; }

// _____________________________________________________________________________________________________________________
const std::string& Network::ip6() const { return _ipv6; }
// _____________________________________________________________________________________________________________________
const std::string& Network::description() const { return _description; }

}  // namespace hwinfo
