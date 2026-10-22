// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/gpu.h"

#include <gtest/gtest.h>

#ifdef HWINFO_WINDOWS
#include "internal/windows_gpu.h"
#endif

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

TEST(Gpu, OrderedByPciAddressThenEnumeration) {
  using hwinfo::Gpu;
  std::vector<Gpu> gpus{
      Gpu{.index = 0, .name = "dGPU", .pci = PciDevice{.vendor_id = 0x10de, .address = "0000:01:00.0"}},
      Gpu{.index = 1, .name = "platform A"},
      Gpu{.index = 2, .name = "iGPU", .pci = PciDevice{.vendor_id = 0x8086, .address = "0000:00:02.0"}},
      Gpu{.index = 3, .name = "no address", .pci = PciDevice{.vendor_id = 0x1002}},
      Gpu{.index = 4, .name = "platform B"},
  };
  order_gpus(gpus);
  std::vector<std::string> names;
  for (std::uint32_t i = 0; i < gpus.size(); ++i) {
    EXPECT_EQ(gpus[i].index, i);
    names.push_back(gpus[i].name);
  }
  EXPECT_EQ(names, (std::vector<std::string>{"iGPU", "dGPU", "platform A", "no address", "platform B"}));
}

TEST(Gpu, ToStringNamesUnknownDevicesByPciIds) {
  using hwinfo::Gpu;
  EXPECT_EQ(hwinfo::to_string(Gpu{.vendor = "NVIDIA", .name = "RTX"}), "NVIDIA RTX");
  EXPECT_EQ(hwinfo::to_string(Gpu{.vendor = "NVIDIA", .pci = PciDevice{.vendor_id = 0x10de, .device_id = 0x2820}}),
            "NVIDIA Device [10de:2820]");
  EXPECT_EQ(hwinfo::to_string(Gpu{.pci = PciDevice{.vendor_id = 0x1234, .device_id = 0x1111}}),
            "Vendor Device [1234:1111]");
  EXPECT_EQ(hwinfo::to_string(Gpu{}), "unknown GPU");
}

#ifdef HWINFO_WINDOWS

TEST(WindowsGpu, ParsesPciIdentityCaseInsensitively) {
  EXPECT_EQ(video_controller_pci("PCI\\VEN_10DE&DEV_2684&SUBSYS_12345678\\4&123"),
            (PciDevice{.vendor_id = 0x10de, .device_id = 0x2684}));
  EXPECT_EQ(video_controller_pci("pci\\ven_8086&dev_a7a0\\3&123"),
            (PciDevice{.vendor_id = 0x8086, .device_id = 0xa7a0}));
  EXPECT_EQ(video_controller_pci("PCI\\VEN_1002&DEV_15BF"), (PciDevice{.vendor_id = 0x1002, .device_id = 0x15bf}));
}

TEST(WindowsGpu, RejectsIncompleteOrNonPciIdentity) {
  EXPECT_FALSE(video_controller_pci(""));
  EXPECT_FALSE(video_controller_pci("ROOT\\VEN_10DE&DEV_2684"));
  EXPECT_FALSE(video_controller_pci("PCI\\VEN_10DE"));
  EXPECT_FALSE(video_controller_pci("PCI\\VEN_ZZZZ&DEV_2684"));
  EXPECT_FALSE(video_controller_pci("PCI\\VEN_10DE&DEV_268"));
  EXPECT_FALSE(video_controller_pci("PCI\\VEN_10DE&DEV_26840"));
}

TEST(WindowsGpu, ReadsWmiFieldsWithoutLosingUnsignedAdapterRam) {
  wmi::Row row;
  row.set("Name", std::string{"NVIDIA GeForce RTX"});
  row.set("AdapterCompatibility", std::string{"NVIDIA"});
  row.set("DriverVersion", std::string{"32.0.15.7680"});
  row.set("PNPDeviceID", std::string{"PCI\\VEN_10DE&DEV_2684"});
  row.set("AdapterRAM", std::uint64_t{0xc0000000});
  const auto gpu = video_controller(row);
  EXPECT_EQ(gpu.name, "NVIDIA GeForce RTX");
  EXPECT_EQ(gpu.vendor, "NVIDIA");
  EXPECT_EQ(gpu.driver_version, "32.0.15.7680");
  EXPECT_EQ(gpu.dedicated_memory, hwinfo::Bytes{0xc0000000});
  EXPECT_EQ(gpu.type, GpuType::discrete);
  EXPECT_EQ(gpu.unified_memory, false);
  EXPECT_FALSE(gpu.shared_memory);
  EXPECT_FALSE(gpu.luid);
}

TEST(WindowsGpu, MissingWmiPropertiesStayUnknown) {
  const auto gpu = video_controller(wmi::Row{});
  EXPECT_TRUE(gpu.name.empty());
  EXPECT_TRUE(gpu.vendor.empty());
  EXPECT_FALSE(gpu.driver_version);
  EXPECT_FALSE(gpu.dedicated_memory);
  EXPECT_FALSE(gpu.pci);
  EXPECT_EQ(gpu.type, GpuType::unknown);
}

TEST(WindowsGpu, MatchesEachIdenticalControllerOnlyOnce) {
  using hwinfo::Gpu;
  const Gpu adapter{.name = "RTX", .luid = 123, .pci = PciDevice{.vendor_id = 0x10de, .device_id = 0x2684}};
  std::vector<Gpu> gpus{
      Gpu{.name = "RTX", .pci = adapter.pci},
      Gpu{.name = "RTX", .pci = adapter.pci},
  };
  auto found = match_video_controller(gpus, adapter);
  ASSERT_EQ(found, gpus.begin());
  found->luid = 123;
  found = match_video_controller(gpus, adapter);
  ASSERT_EQ(found, gpus.begin() + 1);
  found->luid = 456;
  EXPECT_EQ(match_video_controller(gpus, adapter), gpus.end());
}

TEST(WindowsGpu, FallsBackToUniquePciIdsButNotConflictingOrAmbiguousCards) {
  using hwinfo::Gpu;
  const Gpu adapter{.name = "DXGI name", .luid = 123, .pci = PciDevice{.vendor_id = 0x10de, .device_id = 0x2684}};
  std::vector<Gpu> gpus{Gpu{.name = "WMI name", .pci = adapter.pci}};
  EXPECT_EQ(match_video_controller(gpus, adapter), gpus.begin());
  gpus.push_back(gpus.front());
  EXPECT_EQ(match_video_controller(gpus, adapter), gpus.end());
  gpus = {Gpu{.name = adapter.name, .pci = PciDevice{.vendor_id = 0x8086, .device_id = 0xa7a0}}};
  EXPECT_EQ(match_video_controller(gpus, adapter), gpus.end());
}

#ifdef HWINFO_HAS_GPU

TEST(WindowsGpu, DefaultQueryPreservesOsInformation) {
  const auto os = hwinfo::gpus(hwinfo::GpuQuery::os_only());
  ASSERT_TRUE(os) << os.error().message();
  const auto enriched = hwinfo::gpus();
  ASSERT_TRUE(enriched) << enriched.error().message();
  ASSERT_EQ(enriched->size(), os->size());
  for (const auto& gpu : *os) {
    const auto found = std::ranges::find_if(*enriched, [&](const hwinfo::Gpu& candidate) {
      return gpu.luid ? candidate.luid == gpu.luid : candidate.name == gpu.name && candidate.pci == gpu.pci;
    });
    ASSERT_NE(found, enriched->end()) << gpu.name;
    EXPECT_EQ(found->vendor, gpu.vendor);
    EXPECT_EQ(found->name, gpu.name);
    if (gpu.driver_version) {
      EXPECT_EQ(found->driver_version, gpu.driver_version);
    }
    if (gpu.dedicated_memory) {
      EXPECT_EQ(found->dedicated_memory, gpu.dedicated_memory);
    }
    if (gpu.shared_memory) {
      EXPECT_EQ(found->shared_memory, gpu.shared_memory);
    }
    if (gpu.max_frequency) {
      EXPECT_EQ(found->max_frequency, gpu.max_frequency);
    }
  }
}

#endif  // HWINFO_HAS_GPU

#endif  // HWINFO_WINDOWS
