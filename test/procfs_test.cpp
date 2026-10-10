// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include "internal/procfs.h"

#include <gtest/gtest.h>

#include <format>
#include <string>

namespace procfs = hwinfo::internal::procfs;

namespace {

// Two sockets with two cores and SMT each: processors 0..7
std::string x86_cpuinfo() {
  std::string content;
  for (int processor = 0; processor < 8; ++processor) {
    content += std::format(
        "processor\t: {}\n"
        "vendor_id\t: GenuineIntel\n"
        "model name\t: Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz\n"
        "cpu MHz\t\t: 2400.000\n"
        "physical id\t: {}\n"
        "core id\t\t: {}\n"
        "flags\t\t: fpu vme de sse4_2 avx2\n\n",
        processor, processor / 4, processor % 2);
  }
  return content;
}

// Raspberry Pi 4: no vendor or model per processor, global fields at the end
constexpr std::string_view arm_cpuinfo =
    "processor\t: 0\n"
    "BogoMIPS\t: 108.00\n"
    "Features\t: fp asimd evtstrm crc32 cpuid\n"
    "CPU implementer\t: 0x41\n"
    "CPU part\t: 0xd08\n\n"
    "processor\t: 1\n"
    "BogoMIPS\t: 108.00\n"
    "Features\t: fp asimd evtstrm crc32 cpuid\n"
    "CPU implementer\t: 0x41\n"
    "CPU part\t: 0xd08\n\n"
    "Hardware\t: BCM2835\n"
    "Revision\t: c03111\n"
    "Model\t\t: Raspberry Pi 4 Model B Rev 1.1\n";

}  // namespace

TEST(Cpuinfo, X86MultiSocket) {
  const auto processors = procfs::parse_cpuinfo(x86_cpuinfo());
  ASSERT_TRUE(processors) << processors.error().message();
  ASSERT_EQ(processors->size(), 8u);
  const auto& p5 = (*processors)[5];
  EXPECT_EQ(p5.processor, 5u);
  EXPECT_EQ(p5.physical_id, 1u);
  EXPECT_EQ(p5.core_id, 1u);
  EXPECT_EQ(p5.vendor, "GenuineIntel");
  EXPECT_EQ(p5.model, "Intel(R) Xeon(R) CPU E5-2680 v4 @ 2.40GHz");
  EXPECT_EQ(p5.flags, (std::vector<std::string>{"fpu", "vme", "de", "sse4_2", "avx2"}));
  EXPECT_EQ(p5.mhz, 2400.0);
}

TEST(Cpuinfo, ArmUsesImplementerAndGlobalFields) {
  const auto processors = procfs::parse_cpuinfo(arm_cpuinfo);
  ASSERT_TRUE(processors) << processors.error().message();
  ASSERT_EQ(processors->size(), 2u);
  for (const auto& p : *processors) {
    EXPECT_EQ(p.vendor, "ARM");
    EXPECT_EQ(p.model, "Raspberry Pi 4 Model B Rev 1.1");
    EXPECT_EQ(p.physical_id, 0u);
    EXPECT_EQ(p.core_id, p.processor);  // no "core id": every processor is its own core
    EXPECT_EQ(p.flags.size(), 5u);
  }
}

TEST(Cpuinfo, Errors) {
  EXPECT_EQ(procfs::parse_cpuinfo("").error(), hwinfo::errc::parse_error);
  EXPECT_EQ(procfs::parse_cpuinfo("processor : x\n").error(), hwinfo::errc::parse_error);
}

TEST(Stat, ParsesAggregateAndPerCpuTicks) {
  constexpr std::string_view stat =
      "cpu  100 10 50 800 40 5 5 0 0 0\n"
      "cpu0 60 5 30 400 20 3 2 0 0 0\n"
      "cpu1 40 5 20 400 20 2 3 0 0 0\n"
      "intr 12345 0 0\n"
      "ctxt 999\n";
  const auto ticks = procfs::parse_stat(stat);
  ASSERT_TRUE(ticks) << ticks.error().message();
  ASSERT_EQ(ticks->size(), 3u);
  EXPECT_EQ((*ticks)[0].total, 1010u);
  EXPECT_EQ((*ticks)[0].busy, 1010u - 800 - 40);
  EXPECT_EQ((*ticks)[2].total, 490u);
  EXPECT_EQ(procfs::parse_stat("intr 1\n").error(), hwinfo::errc::parse_error);
}

TEST(Stat, IndexesByCpuNumber) {
  // cpu1 is offline: its slot keeps zero ticks so that cpu2 stays at index 1 + 2
  constexpr std::string_view stat =
      "cpu  100 0 0 100 0 0 0 0 0 0\n"
      "cpu0 50 0 0 50 0 0 0 0 0 0\n"
      "cpu2 50 0 0 50 0 0 0 0 0 0\n";
  const auto ticks = procfs::parse_stat(stat);
  ASSERT_TRUE(ticks) << ticks.error().message();
  ASSERT_EQ(ticks->size(), 4u);
  EXPECT_EQ((*ticks)[1].total, 100u);
  EXPECT_EQ((*ticks)[2].total, 0u);
  EXPECT_EQ((*ticks)[3].total, 100u);
  EXPECT_EQ(procfs::parse_stat("cpu0 1 2 3 4\n").error(), hwinfo::errc::parse_error);  // no aggregate line
  EXPECT_EQ(procfs::parse_stat("cpu 1 2 3 4\ncpux 1 2 3 4\n").error(), hwinfo::errc::parse_error);
}

TEST(Meminfo, ParsesKibValues) {
  constexpr std::string_view meminfo =
      "MemTotal:       16000000 kB\n"
      "MemFree:         2000000 kB\n"
      "MemAvailable:    8000000 kB\n"
      "Buffers:          100000 kB\n";
  const auto usage = procfs::parse_meminfo(meminfo);
  ASSERT_TRUE(usage) << usage.error().message();
  EXPECT_EQ(usage->total.value, 16000000ull * 1024);
  EXPECT_EQ(usage->free.value, 2000000ull * 1024);
  EXPECT_EQ(usage->available.value, 8000000ull * 1024);

  // kernels < 3.14 have no MemAvailable
  const auto old = procfs::parse_meminfo("MemTotal: 4 kB\nMemFree: 2 kB\n");
  ASSERT_TRUE(old);
  EXPECT_EQ(old->available, old->free);
  EXPECT_FALSE(procfs::parse_meminfo("Buffers: 1 kB\n"));
}

TEST(OsRelease, UnquotesValuesAndSkipsComments) {
  const auto values = procfs::parse_os_release(
      "# comment\n"
      "NAME=\"Ubuntu\"\n"
      "VERSION=\"24.04.1 LTS (Noble Numbat)\"\n"
      "ID=ubuntu\n"
      "VERSION_ID='24.04'\n");
  EXPECT_EQ(values.at("NAME"), "Ubuntu");
  EXPECT_EQ(values.at("VERSION"), "24.04.1 LTS (Noble Numbat)");
  EXPECT_EQ(values.at("ID"), "ubuntu");
  EXPECT_EQ(values.at("VERSION_ID"), "24.04");
  EXPECT_FALSE(values.contains("# comment"));
}

TEST(OsRelease, Codename) {
  using Values = std::map<std::string, std::string, std::less<>>;
  // display form from VERSION
  EXPECT_EQ(
      procfs::os_release_codename(Values{{"VERSION", "24.04.1 LTS (Noble Numbat)"}, {"VERSION_CODENAME", "noble"}}),
      "Noble Numbat");
  EXPECT_EQ(procfs::os_release_codename(Values{{"VERSION", "12 (bookworm)"}, {"VERSION_CODENAME", "bookworm"}}),
            "bookworm");
  EXPECT_EQ(
      procfs::os_release_codename(Values{{"VERSION", "16.04.7 LTS (Xenial Xerus)"}, {"UBUNTU_CODENAME", "xenial"}}),
      "Xenial Xerus");
  // parentheses not matching the codename
  EXPECT_EQ(procfs::os_release_codename(Values{{"VERSION", "41 (Workstation Edition)"}}), "");
  EXPECT_EQ(procfs::os_release_codename(Values{{"VERSION", "9.4 (Plow)"}, {"VERSION_CODENAME", "other"}}), "other");
  // no VERSION
  EXPECT_EQ(procfs::os_release_codename(Values{{"VERSION_CODENAME", "trixie"}}), "trixie");
  EXPECT_EQ(procfs::os_release_codename(Values{}), "");
}

TEST(Mounts, ParsesAndUnescapes) {
  const auto mounts = procfs::parse_mounts(
      "/dev/nvme0n1p2 / ext4 rw,relatime 0 0\n"
      "/dev/sdb1 /media/My\\040Disk vfat rw 0 0\n"
      "\n"
      "proc /proc proc rw 0 0\n");
  ASSERT_EQ(mounts.size(), 3u);
  EXPECT_EQ(mounts[0].device, "/dev/nvme0n1p2");
  EXPECT_EQ(mounts[0].mount_point, "/");
  EXPECT_EQ(mounts[1].mount_point, "/media/My Disk");
  EXPECT_EQ(mounts[2].device, "proc");
}

TEST(CacheSize, ParsesBinarySuffixes) {
  using namespace hwinfo::literals;
  EXPECT_EQ(procfs::parse_cache_size("48K"), 48_KiB);
  EXPECT_EQ(procfs::parse_cache_size("1280K\n"), 1280_KiB);
  EXPECT_EQ(procfs::parse_cache_size("24576K"), 24_MiB);
  EXPECT_EQ(procfs::parse_cache_size("16M"), 16_MiB);
  EXPECT_EQ(procfs::parse_cache_size("1G"), 1_GiB);
  EXPECT_EQ(procfs::parse_cache_size("512"), 512_B);
  EXPECT_EQ(procfs::parse_cache_size(""), std::nullopt);
  EXPECT_EQ(procfs::parse_cache_size("K"), std::nullopt);
  EXPECT_EQ(procfs::parse_cache_size("12X"), std::nullopt);
}
