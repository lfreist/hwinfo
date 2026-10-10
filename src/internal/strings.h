// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Internal string helpers built on std::string_view and std::ranges. Not part of the public API.

#pragma once

#include <hwinfo/error.h>

#include <algorithm>
#include <charconv>
#include <concepts>
#include <cstdlib>
#include <format>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>

namespace hwinfo::internal {

inline constexpr std::string_view whitespace = " \t\r\n\v\f";

// Removes leading and trailing whitespace.
constexpr std::string_view trim(std::string_view s) noexcept {
  const auto begin = s.find_first_not_of(whitespace);
  if (begin == std::string_view::npos) {
    return {};
  }
  const auto end = s.find_last_not_of(whitespace);
  return s.substr(begin, end - begin + 1);
}

// Removes one pair of surrounding quotes ("..." or '...'), if present.
constexpr std::string_view unquote(std::string_view s) noexcept {
  if (s.size() >= 2 && (s.front() == '"' || s.front() == '\'') && s.back() == s.front()) {
    return s.substr(1, s.size() - 2);
  }
  return s;
}

// Lazily splits `s` at every occurrence of `delimiter` (a char or a string_view). Yields std::string_view.
template <typename Delimiter>
constexpr auto split(std::string_view s, Delimiter delimiter) {
  return std::views::split(s, delimiter) |
         std::views::transform([](auto&& r) { return std::string_view(r.begin(), r.end()); });
}

// Lazily splits `s` into lines (without the trailing '\n').
constexpr auto lines(std::string_view s) { return split(s, '\n'); }

constexpr bool is_space(char c) noexcept { return whitespace.contains(c); }

// Lazily splits `s` at any whitespace, skipping empty tokens.
constexpr auto words(std::string_view s) {
  return s | std::views::chunk_by([](char a, char b) { return is_space(a) == is_space(b); }) |
         std::views::transform([](auto&& r) { return std::string_view(r.begin(), r.end()); }) |
         std::views::filter([](std::string_view w) { return !is_space(w.front()); });
}

// Splits `s` at the first occurrence of `delimiter` into trimmed key and value.
constexpr std::optional<std::pair<std::string_view, std::string_view>> split_key_value(std::string_view s,
                                                                                       char delimiter) noexcept {
  const auto pos = s.find(delimiter);
  if (pos == std::string_view::npos) {
    return std::nullopt;
  }
  return std::pair{trim(s.substr(0, pos)), trim(s.substr(pos + 1))};
}

// Parses a number from the (trimmed) string. The entire string must be consumed.
template <typename T>
  requires std::integral<T> || std::floating_point<T>
result<T> parse(std::string_view s, int base = 10) {
  s = trim(s);
  T value{};
  std::from_chars_result res;
  if constexpr (std::integral<T>) {
    if (base == 16 && (s.starts_with("0x") || s.starts_with("0X"))) {
      s.remove_prefix(2);
    }
    res = std::from_chars(s.data(), s.data() + s.size(), value, base);
  } else {
#if defined(__cpp_lib_to_chars)
    res = std::from_chars(s.data(), s.data() + s.size(), value);
#else
    // libc++ < 20 lacks floating point from_chars
    const std::string copy(s);
    char* end = nullptr;
    value = static_cast<T>(std::strtod(copy.c_str(), &end));
    res = {s.data() + (end - copy.c_str()), end == copy.c_str() ? std::errc::invalid_argument : std::errc{}};
#endif
  }
  if (res.ec != std::errc{} || res.ptr != s.data() + s.size() || s.empty()) {
    return std::unexpected(error{errc::parse_error, std::format("cannot parse '{}' as a number", s)});
  }
  return value;
}

constexpr char to_lower(char c) noexcept { return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c; }

// ASCII case-insensitive comparison.
constexpr bool equals_ignore_case(std::string_view a, std::string_view b) noexcept {
  return std::ranges::equal(a, b, [](char x, char y) { return to_lower(x) == to_lower(y); });
}

constexpr bool starts_with_ignore_case(std::string_view s, std::string_view prefix) noexcept {
  return s.size() >= prefix.size() && equals_ignore_case(s.substr(0, prefix.size()), prefix);
}

constexpr bool contains_ignore_case(std::string_view s, std::string_view needle) noexcept {
  for (std::size_t i = 0; i + needle.size() <= s.size(); ++i) {
    if (equals_ignore_case(s.substr(i, needle.size()), needle)) {
      return true;
    }
  }
  return needle.empty();
}

// Converts an empty or whitespace-only string to std::nullopt.
inline std::optional<std::string> non_empty(std::string_view s) {
  s = trim(s);
  if (s.empty()) {
    return std::nullopt;
  }
  return std::string(s);
}

// Joins the elements of `range` (convertible to std::string_view) with `separator`.
template <std::ranges::input_range R>
std::string join(R&& range, std::string_view separator) {
  std::string out;
  bool first = true;
  for (const auto& element : range) {
    if (!first) {
      out.append(separator);
    }
    out.append(std::string_view(element));
    first = false;
  }
  return out;
}

}  // namespace hwinfo::internal
