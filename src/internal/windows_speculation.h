// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// CPU vulnerabilities from the Windows speculation control information. Pure functions on the flags, so they can be
// unit tested on any platform. Not part of the public API.
//
// Layouts: SYSTEM_SPECULATION_CONTROL_INFORMATION (NtQuerySystemInformation class 201: two ULONGs of flags, the second
// since Windows 11 23H2) and SYSTEM_KERNEL_VA_SHADOW_INFORMATION (class 196: one ULONG), as in phnt's ntexapi.h. The
// interpretation follows Microsoft's SpeculationControl PowerShell module (Get-SpeculationControlSettings).

#pragma once

#include <hwinfo/cpu.h>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace hwinfo::internal::speculation {

enum class Vendor { intel, amd, other };

struct Flags {
  std::uint32_t control = 0;                  // SpeculationControlFlags
  std::optional<std::uint32_t> control2{};    // SpeculationControlFlags2; not reported before 23H2
  std::optional<std::uint32_t> kva_shadow{};  // KvaShadowFlags; not reported before the Meltdown updates
};

constexpr bool bit(std::uint32_t flags, unsigned index) noexcept { return ((flags >> index) & 1u) != 0; }
constexpr std::uint32_t bits(std::uint32_t flags, unsigned index, unsigned count) noexcept {
  return (flags >> index) & ((1u << count) - 1u);
}

// SpeculationControlFlags
inline constexpr unsigned bpb_enabled = 0;
inline constexpr unsigned ssbd_available = 8;
inline constexpr unsigned ssbd_disabled_system_wide = 10;
inline constexpr unsigned ssbd_required = 12;
inline constexpr unsigned hv_l1tf_status_available = 17;
inline constexpr unsigned hv_l1tf_processor_not_affected = 18;
inline constexpr unsigned mds_hardware_protected = 24;
inline constexpr unsigned mb_clear_enabled = 25;
inline constexpr unsigned mb_clear_reported = 26;
// SpeculationControlFlags2
inline constexpr unsigned sbdr_ssdp_hardware_protected = 0;
inline constexpr unsigned fbsdp_hardware_protected = 1;
inline constexpr unsigned psdp_hardware_protected = 2;
inline constexpr unsigned fb_clear_enabled = 3;
inline constexpr unsigned fb_clear_reported = 4;
inline constexpr unsigned branch_confusion_status = 8;  // 2 bits
inline constexpr unsigned branch_confusion_reported = 10;
inline constexpr unsigned rdcl_hardware_protected_reported = 11;
inline constexpr unsigned rdcl_hardware_protected = 12;
inline constexpr unsigned gds_reported = 13;
inline constexpr unsigned gds_status = 14;  // 3 bits
inline constexpr unsigned srso_reported = 17;
inline constexpr unsigned srso_status = 18;  // 2 bits
inline constexpr unsigned rfds_reported = 22;
inline constexpr unsigned rfds_status = 23;  // 2 bits
// KvaShadowFlags
inline constexpr unsigned kva_enabled = 0;
inline constexpr unsigned kva_required = 4;
inline constexpr unsigned kva_required_available = 5;
inline constexpr unsigned kva_invalid_pte_bit = 6;  // 6 bits

// Status fields of SpeculationControlFlags2: 0 mitigation not supported, 1 disabled, 2 hardware immune, 3 mitigated
// (GDS: 4 mitigated and locked).
constexpr VulnerabilityState status_state(std::uint32_t status) noexcept {
  switch (status) {
    case 1:
      return VulnerabilityState::vulnerable;
    case 2:
      return VulnerabilityState::not_affected;
    case 3:
    case 4:
      return VulnerabilityState::mitigated;
    default:
      return VulnerabilityState::unknown;
  }
}

inline std::vector<CpuVulnerability> vulnerabilities(const Flags& flags, Vendor vendor) {
  using enum VulnerabilityState;
  const std::uint32_t f = flags.control;
  const std::uint32_t f2 = flags.control2.value_or(0);
  const bool rdcl_protected = bit(f2, rdcl_hardware_protected_reported) && bit(f2, rdcl_hardware_protected);
  const auto status = [&](unsigned reported, unsigned field, unsigned width) {
    return bit(f2, reported) ? status_state(bits(f2, field, width)) : unknown;
  };

  std::vector<CpuVulnerability> result;
  const auto add = [&](const char* name, VulnerabilityState state) {
    result.push_back({.name = name, .state = state});
  };

  add("gather_data_sampling", status(gds_reported, gds_status, 3));

  // L1TF: Intel only. Mitigated by inverting PTEs (KVA shadow with an invalid PTE bit).
  VulnerabilityState l1tf = unknown;
  if (vendor != Vendor::intel || rdcl_protected ||
      (bit(f, hv_l1tf_status_available) && bit(f, hv_l1tf_processor_not_affected))) {
    l1tf = not_affected;
  } else if (flags.kva_shadow && bit(*flags.kva_shadow, kva_enabled) &&
             bits(*flags.kva_shadow, kva_invalid_pte_bit, 6) != 0) {
    l1tf = mitigated;
  }
  add("l1tf", l1tf);

  // MDS: the PowerShell module treats AMD and ARM as not affected
  VulnerabilityState mds = unknown;
  if (vendor != Vendor::intel) {
    mds = not_affected;
  } else if (bit(f, mb_clear_reported)) {
    mds = bit(f, mds_hardware_protected) ? not_affected : bit(f, mb_clear_enabled) ? mitigated : vulnerable;
  }
  add("mds", mds);

  // Meltdown (rogue data cache load): mitigated by kernel VA shadowing
  VulnerabilityState meltdown = unknown;
  if (rdcl_protected || vendor == Vendor::amd) {
    meltdown = not_affected;
  } else if (const auto kva = flags.kva_shadow) {
    if (bit(*kva, kva_required_available) && !bit(*kva, kva_required)) {
      meltdown = not_affected;
    } else if (bit(*kva, kva_enabled)) {
      meltdown = mitigated;
    } else if (bit(*kva, kva_required_available)) {
      meltdown = vulnerable;
    }
  }
  add("meltdown", meltdown);

  // MMIO stale data (SBDR / FBSDP / PSDP)
  VulnerabilityState mmio = unknown;
  if (vendor != Vendor::intel) {
    mmio = not_affected;
  } else if (bit(f2, fb_clear_reported)) {
    const bool hardware_protected =
        bit(f2, sbdr_ssdp_hardware_protected) && bit(f2, fbsdp_hardware_protected) && bit(f2, psdp_hardware_protected);
    mmio = hardware_protected ? not_affected : bit(f2, fb_clear_enabled) ? mitigated : vulnerable;
  }
  add("mmio_stale_data", mmio);

  add("reg_file_data_sampling", status(rfds_reported, rfds_status, 2));
  add("retbleed", status(branch_confusion_reported, branch_confusion_status, 2));  // AMD branch type confusion
  add("spec_rstack_overflow", status(srso_reported, srso_status, 2));

  // Speculative store bypass: SSBD is opt-in on Windows, "mitigated" only if enabled system wide
  VulnerabilityState ssb = unknown;
  if (bit(f, ssbd_available)) {
    ssb = !bit(f, ssbd_required) ? not_affected : bit(f, ssbd_disabled_system_wide) ? mitigated : vulnerable;
  }
  add("spec_store_bypass", ssb);

  // Branch target injection: Windows reports no "hardware not affected"
  add("spectre_v2", bit(f, bpb_enabled) ? mitigated : vulnerable);

  std::ranges::sort(result, {}, &CpuVulnerability::name);
  return result;
}

}  // namespace hwinfo::internal::speculation
