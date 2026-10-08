// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Internal CoreFoundation / IOKit helpers (macOS): RAII owners and conversions. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_APPLE

#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <hwinfo/error.h>

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "apple_mach.h"
#include "strings.h"

namespace hwinfo::internal::apple {

struct cf_release {
  void operator()(CFTypeRef ref) const noexcept {
    if (ref != nullptr) {
      CFRelease(ref);
    }
  }
};

// Owning reference to a CoreFoundation object obtained by a Create or Copy function, e.g. cf_ptr<CFStringRef>.
template <typename Ref>
using cf_ptr = std::unique_ptr<std::remove_pointer_t<Ref>, cf_release>;

// Owning io_object_t (io_service_t, io_iterator_t, io_registry_entry_t), released with IOObjectRelease.
class io_ptr {
 public:
  io_ptr() noexcept = default;
  explicit io_ptr(io_object_t object) noexcept : _object(object) {}
  io_ptr(io_ptr&& other) noexcept : _object(std::exchange(other._object, IO_OBJECT_NULL)) {}
  io_ptr& operator=(io_ptr&& other) noexcept {
    if (this != &other) {
      reset();
      _object = std::exchange(other._object, IO_OBJECT_NULL);
    }
    return *this;
  }
  io_ptr(const io_ptr&) = delete;
  io_ptr& operator=(const io_ptr&) = delete;
  ~io_ptr() { reset(); }

  [[nodiscard]] io_object_t get() const noexcept { return _object; }
  explicit operator bool() const noexcept { return _object != IO_OBJECT_NULL; }

  void reset() noexcept {
    if (_object != IO_OBJECT_NULL) {
      IOObjectRelease(_object);
      _object = IO_OBJECT_NULL;
    }
  }

 private:
  io_object_t _object = IO_OBJECT_NULL;
};

// CFString, or CFData holding a (NUL terminated) printable string as found in the I/O Registry.
// Trimmed; std::nullopt if empty or of another type.
inline std::optional<std::string> to_string(CFTypeRef value) {
  if (value == nullptr) {
    return std::nullopt;
  }
  if (CFGetTypeID(value) == CFStringGetTypeID()) {
    const auto string = static_cast<CFStringRef>(value);
    if (const char* fast = CFStringGetCStringPtr(string, kCFStringEncodingUTF8); fast != nullptr) {
      return non_empty(fast);
    }
    const CFIndex max_size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(string), kCFStringEncodingUTF8) + 1;
    std::string buffer(static_cast<std::size_t>(max_size), '\0');
    if (!CFStringGetCString(string, buffer.data(), max_size, kCFStringEncodingUTF8)) {
      return std::nullopt;
    }
    return non_empty(buffer.c_str());
  }
  if (CFGetTypeID(value) == CFDataGetTypeID()) {
    const auto data = static_cast<CFDataRef>(value);
    const CFIndex length = CFDataGetLength(data);
    if (length <= 0) {
      return std::nullopt;
    }
    std::string_view s(reinterpret_cast<const char*>(CFDataGetBytePtr(data)), static_cast<std::size_t>(length));
    s = s.substr(0, s.find('\0'));
    if (!std::ranges::all_of(s, [](char c) { return c >= 0x20 && c < 0x7f; })) {
      return std::nullopt;  // binary data
    }
    return non_empty(s);
  }
  return std::nullopt;
}

// CFNumber (integral), or CFData holding a little endian integer of up to 8 bytes (e.g. PCI "vendor-id").
template <typename T>
  requires std::integral<T>
std::optional<T> to_number(CFTypeRef value) {
  if (value == nullptr) {
    return std::nullopt;
  }
  if (CFGetTypeID(value) == CFNumberGetTypeID()) {
    std::int64_t number = 0;
    if (!CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberSInt64Type, &number)) {
      return std::nullopt;  // floating point value or out of range
    }
    return static_cast<T>(number);
  }
  if (CFGetTypeID(value) == CFDataGetTypeID()) {
    const auto data = static_cast<CFDataRef>(value);
    const CFIndex length = CFDataGetLength(data);
    if (length <= 0 || length > 8) {
      return std::nullopt;
    }
    const UInt8* bytes = CFDataGetBytePtr(data);
    std::uint64_t number = 0;
    for (CFIndex i = length; i-- > 0;) {
      number = (number << 8) | bytes[i];
    }
    return static_cast<T>(number);
  }
  return std::nullopt;
}

// CFBoolean, or a CFNumber interpreted as != 0.
inline std::optional<bool> to_bool(CFTypeRef value) {
  if (value != nullptr && CFGetTypeID(value) == CFBooleanGetTypeID()) {
    return CFBooleanGetValue(static_cast<CFBooleanRef>(value)) != 0;
  }
  return to_number<std::int64_t>(value).transform([](std::int64_t n) { return n != 0; });
}

// Borrowed value of a CFDictionary; nullptr if `dictionary` is not a dictionary or has no such key.
inline CFTypeRef dictionary_value(CFTypeRef dictionary, CFStringRef key) {
  if (dictionary == nullptr || CFGetTypeID(dictionary) != CFDictionaryGetTypeID()) {
    return nullptr;
  }
  return CFDictionaryGetValue(static_cast<CFDictionaryRef>(dictionary), key);
}

// Property of the registry entry itself.
inline cf_ptr<CFTypeRef> property(io_registry_entry_t entry, CFStringRef key) {
  return cf_ptr<CFTypeRef>(IORegistryEntryCreateCFProperty(entry, key, kCFAllocatorDefault, 0));
}

// Property of the registry entry or, if it has none, of its closest ancestor in the service plane that has it.
inline cf_ptr<CFTypeRef> search_property(io_registry_entry_t entry, CFStringRef key) {
  return cf_ptr<CFTypeRef>(IORegistryEntrySearchCFProperty(entry, kIOServicePlane, key, kCFAllocatorDefault,
                                                           kIORegistryIterateRecursively | kIORegistryIterateParents));
}

inline std::optional<std::string> string_property(io_registry_entry_t entry, CFStringRef key) {
  return to_string(property(entry, key).get());
}

template <typename T>
  requires std::integral<T>
std::optional<T> number_property(io_registry_entry_t entry, CFStringRef key) {
  return to_number<T>(property(entry, key).get());
}

inline std::optional<bool> bool_property(io_registry_entry_t entry, CFStringRef key) {
  return to_bool(property(entry, key).get());
}

// Parent in the service plane.
// Empty for the root.
inline io_ptr parent(io_registry_entry_t entry) {
  io_registry_entry_t raw = IO_OBJECT_NULL;
  if (IORegistryEntryGetParentEntry(entry, kIOServicePlane, &raw) != KERN_SUCCESS) {
    return {};
  }
  return io_ptr(raw);
}

// Name of the entry's C++ class, e.g. "AGXAcceleratorG13X".
inline std::optional<std::string> class_name(io_object_t object) {
  io_name_t name{};
  if (IOObjectGetClass(object, name) != KERN_SUCCESS) {
    return std::nullopt;
  }
  return non_empty(name);
}

// Name of the registry entry, e.g. "APPLE SSD AP0512Q Media".
inline std::optional<std::string> entry_name(io_registry_entry_t entry) {
  io_name_t name{};
  if (IORegistryEntryGetName(entry, name) != KERN_SUCCESS) {
    return std::nullopt;
  }
  return non_empty(name);
}

// Drains an io_iterator_t.
inline std::vector<io_ptr> collect(io_iterator_t iterator) {
  std::vector<io_ptr> objects;
  while (const io_object_t object = IOIteratorNext(iterator)) {
    objects.emplace_back(object);
  }
  return objects;
}

// All services matching `matching`, which is consumed (IOServiceMatching() result).
// `what` is the error context.
inline result<std::vector<io_ptr>> matching_services(CFMutableDictionaryRef matching, std::string_view what) {
  if (matching == nullptr) {
    return std::unexpected(error{errc::platform_error, std::format("IOServiceMatching({}) failed", what)});
  }
  io_iterator_t raw = IO_OBJECT_NULL;
  const kern_return_t code = IOServiceGetMatchingServices(MACH_PORT_NULL, matching, &raw);
  if (code != KERN_SUCCESS) {
    return std::unexpected(kern_error(code, std::format("IOServiceGetMatchingServices({})", what)));
  }
  const io_ptr iterator(raw);
  return collect(iterator.get());
}

// All services of the given IOKit class (including subclasses).
inline result<std::vector<io_ptr>> matching_services(const char* io_class) {
  return matching_services(IOServiceMatching(io_class), io_class);
}

}  // namespace hwinfo::internal::apple

#endif  // HWINFO_APPLE
