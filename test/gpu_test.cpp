// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/gpu.h"

#include <gtest/gtest.h>

using hwinfo::GpuType;
using hwinfo::PciDevice;
using namespace hwinfo::internal;

TEST(Gpu, PciBus) {
  EXPECT_EQ(pci_bus("0000:00:02.0"), 0u);
  EXPECT_EQ(pci_bus("0000:c1:00.0"), 0xc1u);
  EXPECT_EQ(pci_bus("0000"), std::nullopt);
  EXPECT_EQ(pci_bus("0000:zz:00.0"), std::nullopt);
}

TEST(Gpu, ClassifiesByPciIdentity) {
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = pci_vendor::intel, .device_id = 0xa7a0, .address = "0000:00:02.0"}),
            GpuType::integrated);
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = pci_vendor::intel, .device_id = 0x56a0, .address = "0000:03:00.0"}),
            GpuType::discrete);  // Arc A770
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = pci_vendor::intel, .device_id = 0xa7a0}), GpuType::unknown);
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = pci_vendor::nvidia, .device_id = 0x2860}), GpuType::discrete);
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = pci_vendor::amd, .device_id = 0x15bf}), GpuType::unknown);
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = 0x1af4, .device_id = 0x1050}), GpuType::virtualized);
  EXPECT_EQ(classify_pci_gpu(PciDevice{.vendor_id = 0x15ad, .device_id = 0x0405}), GpuType::virtualized);
}

TEST(Gpu, UnifiedMemoryFollowsType) {
  EXPECT_EQ(unified_memory(GpuType::integrated), true);
  EXPECT_EQ(unified_memory(GpuType::discrete), false);
  EXPECT_EQ(unified_memory(GpuType::virtualized), std::nullopt);
  EXPECT_EQ(unified_memory(GpuType::unknown), std::nullopt);
}
