// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Loading of optional shared libraries (vendor GPU libraries) at runtime. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#include <initializer_list>

#ifdef HWINFO_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace hwinfo::internal {

/**
 * Handle to a shared library loaded at runtime.
 *
 * Libraries are never unloaded: GPU drivers start threads and register callbacks that outlive any API "shutdown"
 * call, so unloading them can crash the process. Every backend loads its library once per process anyway.
 */
class DynamicLibrary {
 public:
  DynamicLibrary() = default;

  // Loads the first library of `names` that can be loaded.
  static DynamicLibrary open(std::initializer_list<const char*> names) noexcept {
    for (const char* name : names) {
#ifdef HWINFO_WINDOWS
      if (HMODULE handle = LoadLibraryExA(name, nullptr, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
        return DynamicLibrary(handle);
      }
#else
      if (void* handle = dlopen(name, RTLD_NOW | RTLD_LOCAL)) {
        return DynamicLibrary(handle);
      }
#endif
    }
    return {};
  }

  explicit operator bool() const noexcept { return _handle != nullptr; }

  template <typename Fn>
  [[nodiscard]] Fn symbol(const char* name) const noexcept {
    if (_handle == nullptr) {
      return nullptr;
    }
#ifdef HWINFO_WINDOWS
    return reinterpret_cast<Fn>(reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(_handle), name)));
#else
    return reinterpret_cast<Fn>(dlsym(_handle, name));
#endif
  }

  template <typename Fn>
  bool load(Fn& fn, const char* name) const noexcept {
    fn = symbol<Fn>(name);
    return fn != nullptr;
  }

 private:
  explicit DynamicLibrary(void* handle) noexcept : _handle(handle) {}

  void* _handle = nullptr;
};

}  // namespace hwinfo::internal
