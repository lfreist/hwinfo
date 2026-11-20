// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include "internal/edac.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

namespace edac = hwinfo::internal::edac;
using hwinfo::MemoryFormFactor;

TEST(Edac, MemoryKind) {
  const auto kind = [](std::string_view text) {
    const auto k = edac::memory_kind(text);
    return std::format("{} {}", k.type.value_or("-"), hwinfo::to_string(k.form_factor));
  };
  EXPECT_EQ(kind("Unbuffered-DDR4\n"), "DDR4 unknown");
  EXPECT_EQ(kind("Registered-DDR5"), "DDR5 DIMM");
  EXPECT_EQ(kind("Load-Reduced-DDR4-RAM"), "DDR4 DIMM");
  EXPECT_EQ(kind("Low-Power-DDR4-RAM"), "LPDDR4 unknown");
  EXPECT_EQ(kind("Low-Power-DDR3-RAM"), "LPDDR3 unknown");
  EXPECT_EQ(kind("FullyBuffered-DDR2"), "DDR2 DIMM");
  EXPECT_EQ(kind("Unbuffered-DDR"), "DDR unknown");
  EXPECT_EQ(kind("Registered-SDR"), "SDRAM DIMM");
  EXPECT_EQ(kind("High-bandwidth-memory-Gen3"), "HBM3 unknown");
  EXPECT_EQ(kind("Unknown"), "- unknown");
  EXPECT_EQ(kind("Empty"), "- unknown");
  EXPECT_EQ(kind(""), "- unknown");
}

namespace {

namespace fs = std::filesystem;

class EdacTree : public ::testing::Test {
 protected:
  void SetUp() override {
    root = fs::temp_directory_path() /
           std::format("hwinfo_edac_{}", ::testing::UnitTest::GetInstance()->current_test_info()->name());
    fs::remove_all(root);
    fs::create_directories(root);
  }
  void TearDown() override { fs::remove_all(root); }

  void dimm(const std::string& path, const std::string& size, const std::string& type, const std::string& label) {
    fs::create_directories(root / path);
    std::ofstream(root / path / "size") << size << '\n';
    std::ofstream(root / path / "dimm_mem_type") << type << '\n';
    std::ofstream(root / path / "dimm_label") << label << '\n';
  }

  fs::path root;
};

}  // namespace

TEST_F(EdacTree, NoController) {
  EXPECT_EQ(edac::memory_modules(root), std::nullopt);
  EXPECT_EQ(edac::memory_modules(root / "missing"), std::nullopt);
}

TEST_F(EdacTree, OnlyEmptySlots) {
  dimm("mc0/dimm0", "0", "Empty", "slot A");
  EXPECT_EQ(edac::memory_modules(root), std::nullopt);
}

TEST_F(EdacTree, Modules) {
  using namespace hwinfo::literals;
  dimm("mc0/dimm0", "16384", "Registered-DDR4", "CPU_SrcID#0_MC#0_Chan#0_DIMM#0");
  dimm("mc0/dimm1", "0", "Empty", "CPU_SrcID#0_MC#0_Chan#0_DIMM#1");
  dimm("mc0/dimm10", "16384", "Registered-DDR4", "CPU_SrcID#0_MC#0_Chan#5_DIMM#0");
  dimm("mc0/dimm2", "16384", "Registered-DDR4", "CPU_SrcID#0_MC#0_Chan#1_DIMM#0");
  dimm("mc1/rank0", "8192", "Unbuffered-DDR3", "mc#1csrow#0channel#0");
  fs::create_directories(root / "power");  // not a controller

  const auto modules = edac::memory_modules(root);
  ASSERT_TRUE(modules);
  ASSERT_EQ(modules->size(), 4u);
  EXPECT_EQ((*modules)[0], (hwinfo::MemoryModule{
                               .index = 0,
                               .name = "CPU_SrcID#0_MC#0_Chan#0_DIMM#0",
                               .size = 16_GiB,
                               .type = "DDR4",
                               .form_factor = MemoryFormFactor::dimm,
                           }));
  EXPECT_EQ((*modules)[1].name, "CPU_SrcID#0_MC#0_Chan#1_DIMM#0");  // dimm2 before dimm10
  EXPECT_EQ((*modules)[2].name, "CPU_SrcID#0_MC#0_Chan#5_DIMM#0");
  EXPECT_EQ((*modules)[3], (hwinfo::MemoryModule{
                               .index = 3,
                               .name = "mc#1csrow#0channel#0",
                               .size = 8_GiB,
                               .type = "DDR3",
                               .form_factor = MemoryFormFactor::unknown,
                           }));
}
