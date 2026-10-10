// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <array>
#include <compare>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace hwinfo {

enum class ByteUnit : std::uint64_t {
  B = 1,
  KiB = 1ull << 10,
  MiB = 1ull << 20,
  GiB = 1ull << 30,
  TiB = 1ull << 40,
  kB = 1'000,
  MB = 1'000'000,
  GB = 1'000'000'000,
  TB = 1'000'000'000'000,
};

enum class FrequencyUnit : std::uint64_t {
  Hz = 1,
  kHz = 1'000,
  MHz = 1'000'000,
  GHz = 1'000'000'000,
};

enum class DataRateUnit : std::uint64_t {
  bps = 1,
  kbps = 1'000,
  Mbps = 1'000'000,
  Gbps = 1'000'000'000,
};

enum class EnergyUnit : std::uint64_t {
  uWh = 1,
  mWh = 1'000,
  Wh = 1'000'000,
};

enum class PowerUnit : std::uint64_t {
  uW = 1,
  mW = 1'000,
  W = 1'000'000,
};

namespace detail {

// Strongly typed unsigned quantity.
// `Unit` is the enum of unit factors used for conversion.
template <typename Tag, typename Unit>
struct quantity {
  using unit_type = Unit;

  std::uint64_t value = 0;

  // Value converted to the given unit, e.g. `Bytes{1536}.to(ByteUnit::KiB) == 1.5`.
  [[nodiscard]] constexpr double to(Unit unit) const noexcept {
    return static_cast<double>(value) / static_cast<double>(std::to_underlying(unit));
  }

  friend constexpr auto operator<=>(quantity, quantity) = default;

  constexpr quantity& operator+=(quantity other) noexcept {
    value += other.value;
    return *this;
  }
  constexpr quantity& operator-=(quantity other) noexcept {
    value -= other.value;
    return *this;
  }
  friend constexpr quantity operator+(quantity a, quantity b) noexcept { return a += b; }
  friend constexpr quantity operator-(quantity a, quantity b) noexcept { return a -= b; }
  friend constexpr quantity operator*(quantity a, std::uint64_t factor) noexcept { return {a.value * factor}; }
  friend constexpr quantity operator*(std::uint64_t factor, quantity a) noexcept { return {a.value * factor}; }
};

struct bytes_tag {};
struct hertz_tag {};
struct data_rate_tag {};
struct energy_tag {};
struct power_tag {};

}  // namespace detail

using Bytes = detail::quantity<detail::bytes_tag, ByteUnit>;
using Hertz = detail::quantity<detail::hertz_tag, FrequencyUnit>;
using DataRate = detail::quantity<detail::data_rate_tag, DataRateUnit>;  // bits per second
using Energy = detail::quantity<detail::energy_tag, EnergyUnit>;         // microwatt-hours
using Power = detail::quantity<detail::power_tag, PowerUnit>;            // microwatts

constexpr Bytes operator*(std::uint64_t value, ByteUnit unit) noexcept { return {value * std::to_underlying(unit)}; }
constexpr Hertz operator*(std::uint64_t value, FrequencyUnit unit) noexcept {
  return {value * std::to_underlying(unit)};
}
constexpr DataRate operator*(std::uint64_t value, DataRateUnit unit) noexcept {
  return {value * std::to_underlying(unit)};
}
constexpr Energy operator*(std::uint64_t value, EnergyUnit unit) noexcept { return {value * std::to_underlying(unit)}; }
constexpr Power operator*(std::uint64_t value, PowerUnit unit) noexcept { return {value * std::to_underlying(unit)}; }

namespace literals {

consteval Bytes operator""_B(unsigned long long v) { return {v}; }
consteval Bytes operator""_KiB(unsigned long long v) { return v * ByteUnit::KiB; }
consteval Bytes operator""_MiB(unsigned long long v) { return v * ByteUnit::MiB; }
consteval Bytes operator""_GiB(unsigned long long v) { return v * ByteUnit::GiB; }
consteval Bytes operator""_TiB(unsigned long long v) { return v * ByteUnit::TiB; }
consteval Bytes operator""_kB(unsigned long long v) { return v * ByteUnit::kB; }
consteval Bytes operator""_MB(unsigned long long v) { return v * ByteUnit::MB; }
consteval Bytes operator""_GB(unsigned long long v) { return v * ByteUnit::GB; }
consteval Bytes operator""_TB(unsigned long long v) { return v * ByteUnit::TB; }

consteval Hertz operator""_Hz(unsigned long long v) { return {v}; }
consteval Hertz operator""_kHz(unsigned long long v) { return v * FrequencyUnit::kHz; }
consteval Hertz operator""_MHz(unsigned long long v) { return v * FrequencyUnit::MHz; }
consteval Hertz operator""_GHz(unsigned long long v) { return v * FrequencyUnit::GHz; }

consteval DataRate operator""_bps(unsigned long long v) { return {v}; }
consteval DataRate operator""_kbps(unsigned long long v) { return v * DataRateUnit::kbps; }
consteval DataRate operator""_Mbps(unsigned long long v) { return v * DataRateUnit::Mbps; }
consteval DataRate operator""_Gbps(unsigned long long v) { return v * DataRateUnit::Gbps; }

consteval Energy operator""_uWh(unsigned long long v) { return {v}; }
consteval Energy operator""_mWh(unsigned long long v) { return v * EnergyUnit::mWh; }
consteval Energy operator""_Wh(unsigned long long v) { return v * EnergyUnit::Wh; }

consteval Power operator""_uW(unsigned long long v) { return {v}; }
consteval Power operator""_mW(unsigned long long v) { return v * PowerUnit::mW; }
consteval Power operator""_W(unsigned long long v) { return v * PowerUnit::W; }

}  // namespace literals

namespace detail {

template <typename Unit>
struct unit_name {
  std::string_view name{};
  Unit unit{};
};

inline constexpr std::array byte_units{
    unit_name<ByteUnit>{"B", ByteUnit::B},     unit_name<ByteUnit>{"KiB", ByteUnit::KiB},
    unit_name<ByteUnit>{"MiB", ByteUnit::MiB}, unit_name<ByteUnit>{"GiB", ByteUnit::GiB},
    unit_name<ByteUnit>{"TiB", ByteUnit::TiB}, unit_name<ByteUnit>{"kB", ByteUnit::kB},
    unit_name<ByteUnit>{"MB", ByteUnit::MB},   unit_name<ByteUnit>{"GB", ByteUnit::GB},
    unit_name<ByteUnit>{"TB", ByteUnit::TB},
};
// Units considered for automatic scaling, ascending.
inline constexpr std::array auto_byte_units{byte_units[0], byte_units[1], byte_units[2], byte_units[3], byte_units[4]};

inline constexpr std::array frequency_units{
    unit_name<FrequencyUnit>{"Hz", FrequencyUnit::Hz},
    unit_name<FrequencyUnit>{"kHz", FrequencyUnit::kHz},
    unit_name<FrequencyUnit>{"MHz", FrequencyUnit::MHz},
    unit_name<FrequencyUnit>{"GHz", FrequencyUnit::GHz},
};

inline constexpr std::array data_rate_units{
    unit_name<DataRateUnit>{"bps", DataRateUnit::bps},
    unit_name<DataRateUnit>{"kbps", DataRateUnit::kbps},
    unit_name<DataRateUnit>{"Mbps", DataRateUnit::Mbps},
    unit_name<DataRateUnit>{"Gbps", DataRateUnit::Gbps},
};

inline constexpr std::array energy_units{
    unit_name<EnergyUnit>{"uWh", EnergyUnit::uWh},
    unit_name<EnergyUnit>{"mWh", EnergyUnit::mWh},
    unit_name<EnergyUnit>{"Wh", EnergyUnit::Wh},
};

inline constexpr std::array power_units{
    unit_name<PowerUnit>{"uW", PowerUnit::uW},
    unit_name<PowerUnit>{"mW", PowerUnit::mW},
    unit_name<PowerUnit>{"W", PowerUnit::W},
};

/**
 * Formatter for quantities. Format spec: [[fill]align][width][.precision][unit]
 *   std::format("{}", 1536_B)        -> "1.5 KiB"   (auto-scaled)
 *   std::format("{:.3MiB}", 1_GiB)   -> "1024.000 MiB"
 *   std::format("{:>12GHz}", hz)     -> "     3.40 GHz"
 */
template <typename Q, const auto& Units, const auto& AutoUnits, int DefaultPrecision>
struct quantity_formatter {
  using unit_type = typename Q::unit_type;

  constexpr auto parse(std::format_parse_context& ctx) {
    auto it = ctx.begin();
    while (it != ctx.end() && *it != '}') {
      ++it;
    }
    std::string_view spec(ctx.begin(), it);

    std::size_t unit_begin = spec.size();
    while (unit_begin > 0 && is_alpha(spec[unit_begin - 1])) {
      --unit_begin;
    }
    if (unit_begin < spec.size()) {
      const std::string_view name = spec.substr(unit_begin);
      bool found = false;
      for (const auto& u : Units) {
        if (u.name == name) {
          _unit = u;
          found = true;
        }
      }
      if (!found) {
        throw std::format_error("hwinfo: unknown unit in format spec");
      }
      spec = spec.substr(0, unit_begin);
    }

    if (const auto dot = spec.rfind('.'); dot != std::string_view::npos && dot + 1 < spec.size()) {
      int precision = 0;
      bool digits = true;
      for (const char c : spec.substr(dot + 1)) {
        digits = digits && c >= '0' && c <= '9';
        precision = precision * 10 + (c - '0');
      }
      if (digits) {
        _precision = precision;
        spec = spec.substr(0, dot);
      }
    }

    std::format_parse_context sub(spec);
    _base.parse(sub);
    return it;
  }

  auto format(const Q& q, std::format_context& ctx) const {
    unit_name<unit_type> unit = _unit.value_or(AutoUnits[0]);
    if (!_unit) {
      for (const auto& u : AutoUnits) {
        if (q.value >= std::to_underlying(u.unit)) {
          unit = u;
        }
      }
    }
    const int precision = _precision.value_or(std::to_underlying(unit.unit) == 1 ? 0 : DefaultPrecision);
    const std::string text = std::format("{:.{}f} {}", q.to(unit.unit), precision, unit.name);
    return _base.format(text, ctx);
  }

 private:
  static constexpr bool is_alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }

  std::formatter<std::string_view> _base;
  std::optional<unit_name<unit_type>> _unit;
  std::optional<int> _precision;
};

}  // namespace detail

}  // namespace hwinfo

template <>
struct std::formatter<hwinfo::Bytes> : hwinfo::detail::quantity_formatter<hwinfo::Bytes, hwinfo::detail::byte_units,
                                                                          hwinfo::detail::auto_byte_units, 1> {};

template <>
struct std::formatter<hwinfo::Hertz>
    : hwinfo::detail::quantity_formatter<hwinfo::Hertz, hwinfo::detail::frequency_units,
                                         hwinfo::detail::frequency_units, 2> {};

template <>
struct std::formatter<hwinfo::DataRate>
    : hwinfo::detail::quantity_formatter<hwinfo::DataRate, hwinfo::detail::data_rate_units,
                                         hwinfo::detail::data_rate_units, 2> {};

template <>
struct std::formatter<hwinfo::Energy> : hwinfo::detail::quantity_formatter<hwinfo::Energy, hwinfo::detail::energy_units,
                                                                           hwinfo::detail::energy_units, 1> {};

template <>
struct std::formatter<hwinfo::Power>
    : hwinfo::detail::quantity_formatter<hwinfo::Power, hwinfo::detail::power_units, hwinfo::detail::power_units, 1> {};
