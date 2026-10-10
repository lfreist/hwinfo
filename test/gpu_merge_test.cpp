// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

// Merging of vendor library (backend) results into the GPU list of the operating system, and the vendor knowledge
// used by the backends.
// Runs without any GPU library.

#include <gtest/gtest.h>

#include <vector>

#include "internal/amdgpu.h"
#include "internal/gpu_backend.h"
#include "internal/gpu_counters.h"
#include "internal/pci.h"

using namespace hwinfo;
using namespace hwinfo::literals;
using hwinfo::internal::gpu::BackendResult;
using hwinfo::internal::gpu::merge;

namespace {

Gpu os_gpu(std::uint16_t vendor, std::uint16_t device, std::optional<std::string> address, GpuType type) {
  return Gpu{.vendor = "OS vendor",
             .name = "OS name",
             .type = type,
             .pci = PciDevice{.vendor_id = vendor, .device_id = device, .address = std::move(address)}};
}

Gpu backend_gpu(std::uint16_t vendor, std::uint16_t device, std::optional<std::string> address) {
  return Gpu{.pci = PciDevice{.vendor_id = vendor, .device_id = device, .address = std::move(address)}};
}

GpuUuid uuid(std::uint8_t first) {
  GpuUuid id;
  id.bytes[0] = first;
  return id;
}

}  // namespace

TEST(GpuMerge, MatchesByPciAddressAndOverridesOsValues) {
  std::vector gpus{os_gpu(0x8086, 0xa7a0, "0000:00:02.0", GpuType::integrated),
                   os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  gpus[1].max_frequency = 1_GHz;
  auto nvidia = backend_gpu(0x10de, 0x2820, "0000:01:00.0");
  nvidia.name = "NVIDIA GeForce RTX 4070 Laptop GPU";
  nvidia.vendor = "NVIDIA";
  nvidia.max_frequency = 3105_MHz;
  nvidia.power_limit = 60_W;
  const std::vector<BackendResult> results{{nvidia}};
  merge(gpus, results);

  EXPECT_EQ(gpus[1].name, "NVIDIA GeForce RTX 4070 Laptop GPU");
  EXPECT_EQ(gpus[1].vendor, "OS vendor");  // the OS (pci.ids) vendor name is kept
  EXPECT_EQ(gpus[1].max_frequency, 3105_MHz);
  EXPECT_EQ(gpus[1].power_limit, 60_W);
  EXPECT_EQ(gpus[0].name, "OS name");  // untouched
  EXPECT_FALSE(gpus[0].power_limit);
}

TEST(GpuMerge, HigherPriorityBackendWins) {
  std::vector gpus{os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  auto nvml = backend_gpu(0x10de, 0x2820, "0000:01:00.0");
  nvml.max_frequency = 3105_MHz;
  auto opencl = backend_gpu(0x10de, 0, "0000:01:00.0");
  opencl.max_frequency = 2000_MHz;
  opencl.l2_cache = 32_MiB;  // only OpenCL knows it
  const std::vector<BackendResult> results{{nvml}, {opencl}};
  merge(gpus, results);

  EXPECT_EQ(gpus[0].max_frequency, 3105_MHz);
  EXPECT_EQ(gpus[0].l2_cache, 32_MiB);
}

TEST(GpuMerge, LaterBackendsMatchByLearnedUuid) {
  std::vector gpus{os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete),
                   os_gpu(0x10de, 0x2820, "0000:02:00.0", GpuType::discrete)};
  auto first = backend_gpu(0x10de, 0x2820, "0000:02:00.0");
  first.uuid = uuid(2);
  Gpu by_uuid;  // e.g. a backend that only reports the UUID
  by_uuid.uuid = uuid(2);
  by_uuid.compute_units = 36;
  const std::vector<BackendResult> results{{first}, {by_uuid}};
  merge(gpus, results);

  EXPECT_EQ(gpus[1].uuid, uuid(2));
  EXPECT_EQ(gpus[1].compute_units, 36u);
  EXPECT_FALSE(gpus[0].compute_units);
}

TEST(GpuMerge, MatchesByLuid) {
  std::vector gpus{os_gpu(0x8086, 0xa7a0, std::nullopt, GpuType::integrated),
                   os_gpu(0x8086, 0xa7a0, std::nullopt, GpuType::integrated)};
  gpus[0].luid = 0x1234;
  gpus[1].luid = 0x5678;
  Gpu device;
  device.luid = 0x5678;
  device.compute_units = 6;
  const std::vector<BackendResult> results{{device}};
  merge(gpus, results);

  EXPECT_FALSE(gpus[0].compute_units);
  EXPECT_EQ(gpus[1].compute_units, 6u);
}

TEST(GpuMerge, IdenticalCardsWithoutAddressAreNotGuessed) {
  std::vector gpus{os_gpu(0x10de, 0x2684, "0000:01:00.0", GpuType::discrete),
                   os_gpu(0x10de, 0x2684, "0000:02:00.0", GpuType::discrete)};
  auto device = backend_gpu(0x10de, 0x2684, std::nullopt);
  device.compute_units = 128;
  const std::vector<BackendResult> results{{device}};
  merge(gpus, results);

  EXPECT_FALSE(gpus[0].compute_units);
  EXPECT_FALSE(gpus[1].compute_units);
}

TEST(GpuMerge, MatchesByUniqueIdsOrVendor) {
  std::vector gpus{os_gpu(0x8086, 0xa7a0, "0000:00:02.0", GpuType::integrated),
                   os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  auto by_ids = backend_gpu(0x8086, 0xa7a0, std::nullopt);
  by_ids.compute_units = 6;
  auto by_vendor = backend_gpu(0x10de, 0, std::nullopt);  // OpenCL without PCI extensions
  by_vendor.compute_units = 36;
  const std::vector<BackendResult> results{{by_ids, by_vendor}};
  merge(gpus, results);

  EXPECT_EQ(gpus[0].compute_units, 6u);
  EXPECT_EQ(gpus[1].compute_units, 36u);
}

TEST(GpuMerge, ConflictingOrUnknownDevicesAreDropped) {
  std::vector gpus{os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  auto other_address = backend_gpu(0x10de, 0x2820, "0000:05:00.0");  // e.g. a GPU without DRM node
  other_address.compute_units = 1;
  Gpu anonymous;  // no identity at all
  anonymous.compute_units = 2;
  const std::vector<BackendResult> results{{other_address, anonymous}};
  merge(gpus, results);

  ASSERT_EQ(gpus.size(), 1u);
  EXPECT_FALSE(gpus[0].compute_units);
}

TEST(GpuMerge, TypeAndUnifiedMemory) {
  std::vector gpus{os_gpu(0x1002, 0x15bf, "0000:c1:00.0", GpuType::unknown),
                   os_gpu(0x1af4, 0x1050, "0000:00:01.0", GpuType::virtualized)};
  auto apu = backend_gpu(0x1002, 0x15bf, "0000:c1:00.0");
  apu.type = GpuType::integrated;
  auto virtio = backend_gpu(0x1af4, 0x1050, "0000:00:01.0");
  virtio.type = GpuType::integrated;  // e.g. a compute driver on a virtual GPU
  const std::vector<BackendResult> results{{apu, virtio}};
  merge(gpus, results);

  EXPECT_EQ(gpus[0].type, GpuType::integrated);
  EXPECT_EQ(gpus[0].unified_memory, true);
  EXPECT_EQ(gpus[1].type, GpuType::virtualized);
}

TEST(GpuMerge, ComputeApisInPriorityOrderWithoutDuplicates) {
  std::vector gpus{os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  auto nvml = backend_gpu(0x10de, 0x2820, "0000:01:00.0");
  nvml.compute_apis = {{"CUDA", "13.2"}};
  auto cuda = nvml;
  auto opencl = backend_gpu(0x10de, 0, "0000:01:00.0");
  opencl.compute_apis = {{"OpenCL", "3.0 CUDA"}};
  const std::vector<BackendResult> results{{nvml}, {cuda}, {opencl}};
  merge(gpus, results);

  EXPECT_EQ(gpus[0].compute_apis, (std::vector<GpuApi>{{"CUDA", "13.2"}, {"OpenCL", "3.0 CUDA"}}));
}

TEST(GpuMerge, PciDetailsAreMergedIntoTheOsPciDevice) {
  std::vector gpus{os_gpu(0x10de, 0x2820, "0000:01:00.0", GpuType::discrete)};
  auto nvml = backend_gpu(0x10de, 0x2820, "0000:01:00.0");
  nvml.pci->max_link = PcieLink{4, 8};
  const std::vector<BackendResult> results{{nvml}};
  merge(gpus, results);

  EXPECT_EQ(gpus[0].pci->max_link, (PcieLink{4, 8}));
  EXPECT_EQ(gpus[0].pci->device_id, 0x2820);
}

TEST(GpuBackends, ParsesIdentifiers) {
  using namespace hwinfo::internal;
  const auto id = gpu::parse_uuid("GPU-86f17d41-e257-95c0-a2aa-8c148ab96b97");
  ASSERT_TRUE(id);
  EXPECT_EQ(std::format("{}", *id), "86f17d41-e257-95c0-a2aa-8c148ab96b97");
  EXPECT_EQ(gpu::parse_uuid("86F17D41E25795C0A2AA8C148AB96B97"), id);
  EXPECT_FALSE(gpu::parse_uuid("GPU-86f17d41"));
  EXPECT_FALSE(gpu::parse_uuid("86f17d41-e257-95c0-a2aa-8c148ab96b97-00"));
  EXPECT_FALSE(gpu::parse_uuid("zzf17d41-e257-95c0-a2aa-8c148ab96b97"));

  EXPECT_EQ(gpu::cuda_version(13020), "13.2");
  EXPECT_EQ(gpu::cuda_version(12080), "12.8");

  EXPECT_EQ(normalize_pci_address("00000000:01:00.0"), "0000:01:00.0");
  EXPECT_EQ(normalize_pci_address("0000:C1:00.1"), "0000:c1:00.1");
  EXPECT_EQ(normalize_pci_address("01:00.0"), "0000:01:00.0");
  EXPECT_FALSE(normalize_pci_address("0000:01:00"));
  EXPECT_FALSE(normalize_pci_address("0000:01:20.0"));

  EXPECT_EQ(pcie_generation("16.0 GT/s PCIe"), 4u);
  EXPECT_EQ(pcie_generation("2.5 GT/s PCIe"), 1u);
  EXPECT_FALSE(pcie_generation("Unknown"));
}

TEST(GpuBackends, NvidiaComputeCapability) {
  Gpu gpu;
  gpu.compute_units = 36;
  internal::gpu::apply_compute_capability(gpu, 8, 9);
  EXPECT_EQ(gpu.compute_capability, "8.9");
  EXPECT_EQ(gpu.architecture, "Ada Lovelace");
  EXPECT_EQ(gpu.cores, 4608u);  // RTX 4070 Laptop GPU

  Gpu a100;
  a100.compute_units = 108;
  internal::gpu::apply_compute_capability(a100, 8, 0);
  EXPECT_EQ(a100.architecture, "Ampere");
  EXPECT_EQ(a100.cores, 6912u);
}

TEST(GpuBackends, IntelIpVersion) {
  Gpu gpu;
  internal::gpu::apply_intel_ip_version(gpu, 12u << 22 | 70u << 14 | 4u);
  EXPECT_EQ(gpu.compute_capability, "12.70.4");
  EXPECT_EQ(gpu.architecture, "Xe-LPG");

  Gpu battlemage;
  internal::gpu::apply_intel_ip_version(battlemage, 20u << 22 | 1u << 14);
  EXPECT_EQ(battlemage.architecture, "Xe2-HPG");
}

TEST(GpuBackends, AmdgpuKfdTopology) {
  using namespace hwinfo::internal::amdgpu;
  EXPECT_EQ(gfx_target(110000), "gfx1100");
  EXPECT_EQ(gfx_target(90010), "gfx90a");
  EXPECT_EQ(gfx_target(100300), "gfx1030");
  EXPECT_EQ(architecture(11, 0, 0), "RDNA 3");
  EXPECT_EQ(architecture(11, 5, 1), "RDNA 3.5");
  EXPECT_EQ(architecture(9, 0, 10), "CDNA 2");
  EXPECT_EQ(architecture(10, 3, 0), "RDNA 2");

  const auto properties = parse_properties(
      "cpu_cores_count 0\n"
      "simd_count 192\n"
      "simd_per_cu 2\n"
      "gfx_target_version 110000\n"
      "location_id 768\n"
      "garbage\n");
  EXPECT_EQ(properties.at("simd_count"), 192u);
  EXPECT_EQ(properties.at("gfx_target_version"), 110000u);
  EXPECT_FALSE(properties.contains("garbage"));
  EXPECT_EQ(kfd_location_id(3, 0, 0), 768u);
}

TEST(GpuCounters, AggregatesEnginesPerAdapterLikeTaskManager) {
  using namespace hwinfo::internal::gpu_counters;
  constexpr std::uint64_t luid = 0x0000d1a6;
  EXPECT_EQ(luid_prefix(luid), "luid_0x00000000_0x0000d1a6");
  const std::vector<std::pair<std::string, double>> engines{
      {"pid_100_luid_0x00000000_0x0000D1A6_phys_0_eng_0_engtype_3D", 30},
      {"pid_200_luid_0x00000000_0x0000D1A6_phys_0_eng_0_engtype_3D", 25},  // same engine: summed
      {"pid_100_luid_0x00000000_0x0000D1A6_phys_0_eng_2_engtype_Compute_0", 40},
      {"pid_300_luid_0x00000000_0x0000D1A6_phys_0_eng_3_engtype_VideoDecode", 12},
      {"pid_100_luid_0x00000000_0x0000AAAA_phys_0_eng_0_engtype_3D", 99},  // another adapter
      {"garbage", 50},
  };
  const auto load = aggregate_engines(engines, luid);
  EXPECT_DOUBLE_EQ(load.utilization.value_or(-1), 0.55);
  EXPECT_DOUBLE_EQ(load.video_utilization.value_or(-1), 0.12);
  EXPECT_FALSE(aggregate_engines(engines, 0x1234).utilization);

  const std::vector<std::pair<std::string, double>> memory{
      {"luid_0x00000000_0x0000D1A6_phys_0", 1024.0 * 1024 * 1024},
      {"luid_0x00000000_0x0000AAAA_phys_0", 5.0},
  };
  EXPECT_EQ(adapter_memory(memory, luid), 1ull << 30);
  EXPECT_FALSE(adapter_memory(memory, 0x1234));
}
