// Copyright Leon Freist
// Authored by Claude (Sonnet 5.5)

#include <gtest/gtest.h>
#include <hwinfo/units.h>

#include <format>

using namespace hwinfo::literals;
using hwinfo::Bytes;
using hwinfo::ByteUnit;
using hwinfo::DataRate;
using hwinfo::DataRateUnit;
using hwinfo::Energy;
using hwinfo::EnergyUnit;
using hwinfo::FrequencyUnit;
using hwinfo::Hertz;
using hwinfo::Power;
using hwinfo::PowerUnit;

TEST(Units, Literals) {
  static_assert(1_KiB == Bytes{1024});
  static_assert(2_GiB == 2048_MiB);
  static_assert(1_GB == Bytes{1'000'000'000});
  static_assert(3400_MHz == Hertz{3'400'000'000});
  static_assert(1_TiB > 1_TB);
  static_assert(1_GiB + 1_GiB - 1_GiB == 1_GiB);
  static_assert(4 * 1_KiB == 4_KiB);
  static_assert(5_Gbps == DataRate{5'000'000'000});
  static_assert(480_Mbps < 5_Gbps);
  static_assert(1_Wh == 1000_mWh);
  static_assert(52_Wh == Energy{52'000'000});
  static_assert(60_W == Power{60'000'000});
  static_assert(1_W == 1000_mW);
}

TEST(Units, Conversion) {
  EXPECT_DOUBLE_EQ((1536_B).to(ByteUnit::KiB), 1.5);
  EXPECT_DOUBLE_EQ((1_GiB).to(ByteUnit::MiB), 1024.0);
  EXPECT_DOUBLE_EQ((1_GHz).to(FrequencyUnit::MHz), 1000.0);
  EXPECT_DOUBLE_EQ((480_Mbps).to(DataRateUnit::Gbps), 0.48);
  EXPECT_DOUBLE_EQ((52500_mWh).to(EnergyUnit::Wh), 52.5);
  EXPECT_DOUBLE_EQ((12500_mW).to(PowerUnit::W), 12.5);
}

TEST(Units, FormatAutoScales) {
  EXPECT_EQ(std::format("{}", 0_B), "0 B");
  EXPECT_EQ(std::format("{}", 512_B), "512 B");
  EXPECT_EQ(std::format("{}", 1536_B), "1.5 KiB");
  EXPECT_EQ(std::format("{}", 16_GiB), "16.0 GiB");
  EXPECT_EQ(std::format("{}", 2_TiB), "2.0 TiB");
  EXPECT_EQ(std::format("{}", 3400_MHz), "3.40 GHz");
  EXPECT_EQ(std::format("{}", 800_kHz), "800.00 kHz");
  EXPECT_EQ(std::format("{}", 5_Gbps), "5.00 Gbps");
  EXPECT_EQ(std::format("{}", 12_Mbps), "12.00 Mbps");
  EXPECT_EQ(std::format("{}", 52500_mWh), "52.5 Wh");
  EXPECT_EQ(std::format("{}", 800_mWh), "800.0 mWh");
  EXPECT_EQ(std::format("{}", 60_W), "60.0 W");
  EXPECT_EQ(std::format("{}", 250_mW), "250.0 mW");
}

TEST(Units, FormatSpec) {
  EXPECT_EQ(std::format("{:MiB}", 1_GiB), "1024.0 MiB");
  EXPECT_EQ(std::format("{:.3GB}", 1_GiB), "1.074 GB");
  EXPECT_EQ(std::format("{:.0B}", 1_KiB), "1024 B");
  EXPECT_EQ(std::format("{:>10}", 1_KiB), "   1.0 KiB");
  EXPECT_EQ(std::format("{:<10.1GHz}|", 3400_MHz), "3.4 GHz   |");
  EXPECT_EQ(std::format("{:*^14MHz}", 1_GHz), "*1000.00 MHz**");
  EXPECT_EQ(std::format("{:.0Mbps}", 5_Gbps), "5000 Mbps");
  EXPECT_EQ(std::format("{:.2Wh}", 52500_mWh), "52.50 Wh");
}
