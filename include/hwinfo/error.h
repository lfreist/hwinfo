// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#pragma once

#include <hwinfo/platform.h>

#include <expected>
#include <format>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

namespace hwinfo {

/**
 * Portable error conditions reported by hwinfo.
 *
 * Errors keep the std::error_code they originate from (errno values, Win32 errors, IOKit return codes, ...).
 * These conditions are what you compare against: platform specific codes are mapped to them, e.g.
 * `EACCES == errc::permission_denied`.
 */
enum class errc {
  not_supported = 1,  // the platform does not provide this information
  not_found,          // the source of the information does not exist (missing file, device, ...)
  permission_denied,  // insufficient privileges
  io_error,           // reading the information failed
  parse_error,        // the information could not be parsed
  platform_error,     // any other error reported by an OS API
};

namespace detail {

class error_category final : public std::error_category {
 public:
  static constexpr const char* category_name = "hwinfo";

  [[nodiscard]] const char* name() const noexcept override { return category_name; }

  [[nodiscard]] std::string message(int value) const override {
    switch (static_cast<errc>(value)) {
      case errc::not_supported:
        return "not supported on this platform";
      case errc::not_found:
        return "not found";
      case errc::permission_denied:
        return "permission denied";
      case errc::io_error:
        return "I/O error";
      case errc::parse_error:
        return "parse error";
      case errc::platform_error:
        return "platform error";
    }
    return "unknown hwinfo error";
  }

  // Maps foreign error codes (errno, Win32, ...) onto hwinfo::errc conditions.
  [[nodiscard]] bool equivalent(const std::error_code& code, int condition) const noexcept override {
    if (is_hwinfo(code.category())) {
      return code.value() == condition;
    }
    return static_cast<int>(to_errc(code)) == condition;
  }

  [[nodiscard]] bool equivalent(int value, const std::error_condition& condition) const noexcept override {
    return is_hwinfo(condition.category()) && value == condition.value();
  }

 private:
  // The category is compared by name instead of by address: each shared library holds its own instance.
  static bool is_hwinfo(const std::error_category& cat) noexcept {
    return std::string_view(cat.name()) == category_name;
  }

  static errc to_errc(const std::error_code& code) noexcept {
    const std::error_condition cond = code.default_error_condition();
    if (cond == std::errc::permission_denied || cond == std::errc::operation_not_permitted) {
      return errc::permission_denied;
    }
    if (cond == std::errc::no_such_file_or_directory || cond == std::errc::no_such_device ||
        cond == std::errc::no_such_device_or_address) {
      return errc::not_found;
    }
    if (cond == std::errc::not_supported || cond == std::errc::operation_not_supported ||
        cond == std::errc::function_not_supported) {
      return errc::not_supported;
    }
    if (cond == std::errc::io_error || cond.category() == std::generic_category()) {
      return errc::io_error;
    }
    return errc::platform_error;
  }
};

}  // namespace detail

inline const std::error_category& category() noexcept {
  static const detail::error_category instance;
  return instance;
}

inline std::error_code make_error_code(errc e) noexcept { return {std::to_underlying(e), category()}; }
inline std::error_condition make_error_condition(errc e) noexcept { return {std::to_underlying(e), category()}; }

/**
 * Error type of all hwinfo queries: a std::error_code plus an optional context (e.g. the path that could not be read).
 */
class error {
 public:
  error(std::error_code code, std::string context = {}) : _code(code), _context(std::move(context)) {}
  error(errc e, std::string context = {}) : error(make_error_code(e), std::move(context)) {}

  [[nodiscard]] const std::error_code& code() const noexcept { return _code; }
  [[nodiscard]] const std::string& context() const noexcept { return _context; }

  // "<context>: <description of code>"
  [[nodiscard]] std::string message() const {
    return _context.empty() ? _code.message() : _context + ": " + _code.message();
  }

  friend bool operator==(const error& e, errc condition) noexcept { return e._code == make_error_condition(condition); }
  friend bool operator==(const error& e, std::errc condition) noexcept { return e._code == condition; }

 private:
  std::error_code _code;
  std::string _context;
};

template <typename T>
using result = std::expected<T, error>;

}  // namespace hwinfo

template <>
struct std::is_error_condition_enum<hwinfo::errc> : std::true_type {};

template <>
struct std::formatter<hwinfo::error> : std::formatter<std::string> {
  auto format(const hwinfo::error& e, std::format_context& ctx) const {
    return std::formatter<std::string>::format(e.message(), ctx);
  }
};
