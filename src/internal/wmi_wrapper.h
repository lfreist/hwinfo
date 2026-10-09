// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Minimal WMI (Windows Management Instrumentation) client. Not part of the public API.
//
//   auto rows = internal::wmi::query("Win32_BaseBoard", {"Manufacturer", "Product"});
//   if (rows && !rows->empty()) { auto vendor = rows->front().string("Manufacturer"); ... }

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <WbemIdl.h>
#include <hwinfo/error.h>

#include <concepts>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "internal/strings.h"
#include "internal/windows_com.h"

#ifdef _MSC_VER
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#endif

namespace hwinfo::internal::wmi {

// Value of a WMI property. 64 bit integers are transferred as strings by WMI and are kept as such.
using Value = std::variant<std::string, std::int64_t, std::uint64_t, double, bool, std::vector<std::string>>;

// One object of a query result: the requested properties that are not NULL.
class Row {
 public:
  void set(std::string property, Value value) { _values.insert_or_assign(std::move(property), std::move(value)); }

  [[nodiscard]] const Value* find(std::string_view property) const {
    const auto it = _values.find(property);
    return it == _values.end() ? nullptr : &it->second;
  }

  // String property, trimmed; std::nullopt if NULL or empty.
  [[nodiscard]] std::optional<std::string> string(std::string_view property) const {
    const auto* value = find(property);
    const auto* s = value != nullptr ? std::get_if<std::string>(value) : nullptr;
    return s != nullptr ? non_empty(*s) : std::nullopt;
  }

  // Integer property (also parses the string representation WMI uses for 64 bit integers).
  // std::nullopt if NULL or not representable as T.
  template <std::integral T>
    requires(!std::same_as<T, bool>)
  [[nodiscard]] std::optional<T> number(std::string_view property) const {
    const auto* value = find(property);
    if (value == nullptr) {
      return std::nullopt;
    }
    if (const auto* s = std::get_if<std::string>(value)) {
      const auto parsed = parse<T>(*s);
      return parsed ? std::optional<T>(*parsed) : std::nullopt;
    }
    if (const auto* i = std::get_if<std::int64_t>(value); i != nullptr && std::in_range<T>(*i)) {
      return static_cast<T>(*i);
    }
    if (const auto* u = std::get_if<std::uint64_t>(value); u != nullptr && std::in_range<T>(*u)) {
      return static_cast<T>(*u);
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<bool> boolean(std::string_view property) const {
    const auto* value = find(property);
    const auto* b = value != nullptr ? std::get_if<bool>(value) : nullptr;
    return b != nullptr ? std::optional<bool>(*b) : std::nullopt;
  }

  // String array property; empty if NULL.
  [[nodiscard]] std::vector<std::string> strings(std::string_view property) const {
    const auto* value = find(property);
    const auto* v = value != nullptr ? std::get_if<std::vector<std::string>>(value) : nullptr;
    return v != nullptr ? *v : std::vector<std::string>{};
  }

 private:
  std::map<std::string, Value, std::less<>> _values;
};

// Connection to a WMI namespace.
// Initializes COM on the calling thread for its lifetime: use it on one thread only.
class Connection {
 public:
  static result<Connection> connect(std::wstring_view wmi_namespace = L"ROOT\\CIMV2");

  // SELECT <properties> FROM <wmi_class> [WHERE <filter>]
  [[nodiscard]] result<std::vector<Row>> query(std::string_view wmi_class,
                                               std::initializer_list<std::string_view> properties,
                                               std::string_view filter = {}) const;

 private:
  Connection() = default;

  ComInit _com;
  ComPtr<IWbemLocator> _locator;
  ComPtr<IWbemServices> _services;
};

// Connects to `wmi_namespace` and runs a single query.
[[nodiscard]] inline result<std::vector<Row>> query(std::string_view wmi_class,
                                                    std::initializer_list<std::string_view> properties,
                                                    std::string_view filter = {},
                                                    std::wstring_view wmi_namespace = L"ROOT\\CIMV2") {
  return Connection::connect(wmi_namespace).and_then([&](const Connection& connection) {
    return connection.query(wmi_class, properties, filter);
  });
}

}  // namespace hwinfo::internal::wmi

#endif  // HWINFO_WINDOWS
