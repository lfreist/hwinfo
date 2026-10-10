// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/pci.h"

#include <gtest/gtest.h>

using hwinfo::internal::lookup_pci;

namespace {

constexpr std::string_view database =
    "# comment\n"
    "\n"
    "10de  NVIDIA Corporation\n"
    "\t2786  AD104 [GeForce RTX 4070]\n"
    "\t\t1043 88f2  TUF Gaming RTX 4070\n"
    "\t2860  AD106M [GeForce RTX 4070 Max-Q / Mobile]\n"
    "8086  Intel Corporation\n"
    "\ta7a0  Raptor Lake-P [Iris Xe Graphics]\n"
    "C 03  Display controller\n"
    "\t00  VGA compatible controller\n";

}  // namespace

TEST(Pci, FindsVendorAndDevice) {
  const auto names = lookup_pci(database, 0x10de, 0x2860);
  EXPECT_EQ(names.vendor, "NVIDIA Corporation");
  EXPECT_EQ(names.device, "AD106M [GeForce RTX 4070 Max-Q / Mobile]");

  const auto intel = lookup_pci(database, 0x8086, 0xa7a0);
  EXPECT_EQ(intel.vendor, "Intel Corporation");
  EXPECT_EQ(intel.device, "Raptor Lake-P [Iris Xe Graphics]");
}

TEST(Pci, DoesNotMatchSubsystemsOrOtherVendors) {
  const auto unknown_device = lookup_pci(database, 0x10de, 0x88f2);  // only a subsystem id
  EXPECT_EQ(unknown_device.vendor, "NVIDIA Corporation");
  EXPECT_EQ(unknown_device.device, std::nullopt);

  const auto wrong_vendor = lookup_pci(database, 0x10de, 0xa7a0);  // Intel device id
  EXPECT_EQ(wrong_vendor.device, std::nullopt);

  const auto unknown = lookup_pci(database, 0x1234, 0x0000);
  EXPECT_EQ(unknown.vendor, std::nullopt);
  EXPECT_EQ(unknown.device, std::nullopt);
}

TEST(Pci, FormatsAddress) {
  EXPECT_EQ(hwinfo::internal::format_pci_address(0, 1, 0, 0), "0000:01:00.0");
  EXPECT_EQ(hwinfo::internal::format_pci_address(0x10, 0xc1, 0x1f, 7), "0010:c1:1f.7");
}

TEST(Pci, ParsesPcidebug) {
  using hwinfo::internal::parse_pcidebug;
  EXPECT_EQ(parse_pcidebug("0:2:0"), "0000:00:02.0");
  EXPECT_EQ(parse_pcidebug("193:0:1(194:194)"), "0000:c1:00.1");
  EXPECT_EQ(parse_pcidebug(""), std::nullopt);
  EXPECT_EQ(parse_pcidebug("1:0"), std::nullopt);
  EXPECT_EQ(parse_pcidebug("1:0:0:0"), std::nullopt);
  EXPECT_EQ(parse_pcidebug("1:32:0"), std::nullopt);
}
