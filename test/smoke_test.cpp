// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

// Runs every query on the machine executing the tests. Hardware differs between machines (CI runners are VMs without
// batteries or GPUs), so these tests only check invariants: a query either succeeds with plausible values or reports
// a well-formed error. They never crash.

#include <gtest/gtest.h>
#include <hwinfo/hwinfo.h>

#include <algorithm>
#include <format>
#include <thread>

#ifdef __linux__
#include <sched.h>
#endif

using namespace std::chrono_literals;

namespace {

template <typename T>
void expect_value_or_error(const hwinfo::result<T>& r) {
  if (!r) {
    EXPECT_FALSE(r.error().message().empty());
    std::cout << "[          ] query failed: " << r.error().message() << '\n';
  }
}

}  // namespace

#ifdef HWINFO_HAS_CPU
TEST(Smoke, Cpus) {
  const auto cpus = hwinfo::cpus();
  ASSERT_TRUE(cpus) << cpus.error().message();
  ASSERT_FALSE(cpus->empty());
  for (const auto& cpu : *cpus) {
    EXPECT_GT(cpu.physical_cores, 0u);
    EXPECT_GE(cpu.logical_cores, cpu.physical_cores);
    EXPECT_EQ(cpu.cores.size(), cpu.physical_cores);
    EXPECT_FALSE(std::format("{}", cpu).empty());
    for (const auto& core : cpu.cores) {
      if (!core.logical_ids.empty()) {  // empty on macOS
        EXPECT_EQ(core.logical_ids.size(), core.threads);
        EXPECT_TRUE(std::ranges::is_sorted(core.logical_ids));
      }
    }
  }
}

#ifdef __linux__
// logical_ids are the numbers sched_setaffinity takes: a thread pinned to one of them runs on exactly that CPU.
TEST(Smoke, LogicalIdsArePinnable) {
  const auto cpus = hwinfo::cpus();
  ASSERT_TRUE(cpus) << cpus.error().message();
  cpu_set_t allowed;
  ASSERT_EQ(sched_getaffinity(0, sizeof(allowed), &allowed), 0);
  std::thread([&] {
    for (const auto& cpu : *cpus) {
      for (const auto& core : cpu.cores) {
        for (const std::uint32_t id : core.logical_ids) {
          if (!CPU_ISSET(id, &allowed)) {
            continue;  // e.g. restricted by a container's cpuset
          }
          cpu_set_t set;
          CPU_ZERO(&set);
          CPU_SET(id, &set);
          ASSERT_EQ(sched_setaffinity(0, sizeof(set), &set), 0) << "cpu " << id;
          EXPECT_EQ(sched_getcpu(), static_cast<int>(id));
        }
      }
    }
  }).join();
}
#endif

TEST(Smoke, CpuSampler) {
  hwinfo::CpuSampler sampler;
  std::this_thread::sleep_for(50ms);
  const auto load = sampler.sample();
  ASSERT_TRUE(load) << load.error().message();
  EXPECT_GE(load->total, 0.0);
  EXPECT_LE(load->total, 1.0);
  EXPECT_FALSE(load->per_thread.empty());
  for (const double u : load->per_thread) {
    EXPECT_GE(u, 0.0);
    EXPECT_LE(u, 1.0);
  }
  const auto frequencies = hwinfo::cpu_frequencies();
  expect_value_or_error(frequencies);
#ifdef __linux__
  if (frequencies) {
    EXPECT_EQ(frequencies->size(), load->per_thread.size());  // both indexed by OS CPU number
  }
#endif
}
#endif

#ifdef HWINFO_HAS_RAM
TEST(Smoke, Memory) {
  const auto memory = hwinfo::memory();
  ASSERT_TRUE(memory) << memory.error().message();
  EXPECT_GT(memory->total.value, 0u);

  const auto usage = hwinfo::memory_usage();
  ASSERT_TRUE(usage) << usage.error().message();
  EXPECT_LE(usage->free, usage->total);
  EXPECT_LE(usage->available, usage->total);
}
#endif

#ifdef HWINFO_HAS_OS
TEST(Smoke, Os) {
  const auto os = hwinfo::os();
  ASSERT_TRUE(os) << os.error().message();
  EXPECT_FALSE(os->name.empty());
  EXPECT_TRUE(os->bits == 32 || os->bits == 64);
}
#endif

#ifdef HWINFO_HAS_DISK
TEST(Smoke, Disks) {
  const auto disks = hwinfo::disks();
  expect_value_or_error(disks);
  for (const auto& disk : disks.value_or(std::vector<hwinfo::Disk>{})) {
    for (const auto& mount_point : disk.mount_points) {
      expect_value_or_error(hwinfo::disk_space(mount_point));
    }
  }
}
#endif

TEST(Smoke, DiskSpaceOfMissingPathIsAnError) {
  const auto space = hwinfo::disk_space("/this/path/does/not/exist");
  ASSERT_FALSE(space);
  EXPECT_EQ(space.error(), hwinfo::errc::not_found);
}

#ifdef HWINFO_HAS_GPU
TEST(Smoke, GpusWithoutVendorLibraries) {
  const auto gpus = hwinfo::gpus(hwinfo::GpuQuery::os_only());
  expect_value_or_error(gpus);
  if (gpus) {
    for (const auto& gpu : *gpus) {
      EXPECT_TRUE(gpu.compute_apis.empty());
    }
  }
}

TEST(Smoke, GpuSampler) {
  const auto gpus = hwinfo::gpus();
  if (!gpus) {
    GTEST_SKIP() << gpus.error().message();
  }
  for (const auto& gpu : *gpus) {
    hwinfo::GpuSampler sampler(gpu);
    std::this_thread::sleep_for(50ms);
    const auto status = sampler.sample();
    if (!status) {
      EXPECT_EQ(status.error(), hwinfo::errc::not_supported) << status.error().message();
      continue;
    }
    for (const auto& fraction :
         {status->utilization, status->memory_utilization, status->video_utilization, status->fan_speed}) {
      if (fraction) {
        EXPECT_GE(*fraction, 0.0);
        EXPECT_LE(*fraction, 1.0);
      }
    }
    if (status->memory_used && status->memory_total) {
      EXPECT_LE(*status->memory_used, *status->memory_total);
    }
  }
}

TEST(Smoke, Gpus) {
  const auto gpus = hwinfo::gpus();
  expect_value_or_error(gpus);
  if (!gpus) {
    return;
  }
  for (const auto& gpu : *gpus) {
    EXPECT_FALSE(gpu.name.empty());
    EXPECT_FALSE(std::format("{} ({})", gpu, gpu.type).empty());
    if (gpu.unified_memory) {
      EXPECT_EQ(*gpu.unified_memory, gpu.type == hwinfo::GpuType::integrated);
    }
    if (gpu.pci && gpu.pci->address) {
      EXPECT_EQ(gpu.pci->address->size(), 12u) << *gpu.pci->address;  // "0000:01:00.0"
    }
    if (gpu.cores && gpu.compute_units) {
      EXPECT_GE(*gpu.cores, *gpu.compute_units);
    }
    for (const auto& api : gpu.compute_apis) {
      EXPECT_FALSE(api.name.empty());
    }
  }
}
#endif

#ifdef HWINFO_HAS_COMPUTER
TEST(Smoke, Computer) {
  const auto computer = hwinfo::computer();
  expect_value_or_error(computer);
  if (computer) {
    EXPECT_FALSE(std::format("{} ({})", *computer, computer->chassis).empty());
  }
}
#endif

#if defined(HWINFO_HAS_COMPUTER) && defined(HWINFO_HAS_OS)
TEST(Smoke, ComputerForwardsToComponents) {
  const auto computer = hwinfo::computer();
  if (!computer) {
    GTEST_SKIP() << computer.error().message();
  }
  const auto via_computer = computer->os();
  const auto direct = hwinfo::os();
  ASSERT_EQ(via_computer.has_value(), direct.has_value());
  if (direct) {
    EXPECT_EQ(*via_computer, *direct);
  }
}
#endif

#ifdef HWINFO_HAS_MAINBOARD
TEST(Smoke, Mainboard) { expect_value_or_error(hwinfo::mainboard()); }
#endif

#ifdef HWINFO_HAS_BATTERY
TEST(Smoke, Batteries) {
  const auto batteries = hwinfo::batteries();
  expect_value_or_error(batteries);
  for (const auto& battery : batteries.value_or(std::vector<hwinfo::Battery>{})) {
    const auto status = hwinfo::battery_status(battery.index);
    expect_value_or_error(status);
    if (status && status->charge) {
      EXPECT_GE(*status->charge, 0.0);
      EXPECT_LE(*status->charge, 1.0);
    }
  }
  const auto missing = hwinfo::battery_status(1000);
  ASSERT_FALSE(missing);
}
#endif

#ifdef HWINFO_HAS_NETWORK
TEST(Smoke, NetworkInterfaces) {
  const auto nics = hwinfo::network_interfaces();
  ASSERT_TRUE(nics) << nics.error().message();
  for (const auto& nic : *nics) {
    EXPECT_FALSE(nic.name.empty());
  }
}
#endif
