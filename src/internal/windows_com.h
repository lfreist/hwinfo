// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

// Minimal RAII helpers for COM. Not part of the public API.

#pragma once

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

#include <objbase.h>

#include <utility>

namespace hwinfo::internal {

// Owning pointer to a COM interface: releases its reference on destruction.
template <typename T>
class ComPtr {
 public:
  ComPtr() = default;
  ComPtr(const ComPtr&) = delete;
  ComPtr& operator=(const ComPtr&) = delete;
  ComPtr(ComPtr&& other) noexcept : _ptr(std::exchange(other._ptr, nullptr)) {}
  ComPtr& operator=(ComPtr&& other) noexcept {
    if (this != &other) {
      reset();
      _ptr = std::exchange(other._ptr, nullptr);
    }
    return *this;
  }
  ~ComPtr() { reset(); }

  void reset() noexcept {
    if (_ptr != nullptr) {
      _ptr->Release();
      _ptr = nullptr;
    }
  }

  // Releases the current interface and returns the address of the pointer, to be filled by a COM API.
  [[nodiscard]] T** put() noexcept {
    reset();
    return &_ptr;
  }
  // Same as put(), typed for APIs taking a `void**` (IID_PPV_ARGS style).
  [[nodiscard]] void** put_void() noexcept { return reinterpret_cast<void**>(put()); }

  [[nodiscard]] T* get() const noexcept { return _ptr; }
  T* operator->() const noexcept { return _ptr; }
  explicit operator bool() const noexcept { return _ptr != nullptr; }

 private:
  T* _ptr = nullptr;
};

// Initializes COM on the calling thread for the lifetime of the object. If the thread already uses COM in another
// concurrency model (RPC_E_CHANGED_MODE), COM is usable but must not be uninitialized by us.
class ComInit {
 public:
  ComInit() : _hr(CoInitializeEx(nullptr, COINIT_MULTITHREADED)) {}
  ComInit(const ComInit&) = delete;
  ComInit& operator=(const ComInit&) = delete;
  ComInit(ComInit&& other) noexcept : _hr(std::exchange(other._hr, E_FAIL)) {}
  ComInit& operator=(ComInit&& other) noexcept {
    if (this != &other) {
      release();
      _hr = std::exchange(other._hr, E_FAIL);
    }
    return *this;
  }
  ~ComInit() { release(); }

  // S_OK / S_FALSE: initialized by us; RPC_E_CHANGED_MODE: already initialized by someone else.
  [[nodiscard]] bool usable() const noexcept { return SUCCEEDED(_hr) || _hr == RPC_E_CHANGED_MODE; }
  [[nodiscard]] HRESULT status() const noexcept { return _hr; }

 private:
  void release() noexcept {
    if (SUCCEEDED(_hr)) {
      CoUninitialize();
    }
    _hr = E_FAIL;
  }

  HRESULT _hr;
};

}  // namespace hwinfo::internal

#endif  // HWINFO_WINDOWS
