// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <dxgi.h>
// clang-format on
#include <hwinfo/gpu.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/pci.h"
#include "internal/windows_com.h"
#include "internal/windows_error.h"
#include "internal/windows_strings.h"
#include "pci.ids.h"

#ifdef USE_OCL
#include "opencl/device.h"
#endif

#ifdef _MSC_VER
#pragma comment(lib, "dxgi.lib")
#endif

namespace hwinfo {

namespace {

std::string_view pci_database() { return {reinterpret_cast<const char*>(pci_ids), pci_ids_size}; }

std::string vendor_name(std::uint16_t vendor_id) {
  if (auto name = internal::lookup_pci(pci_database(), vendor_id, 0).vendor) {
    return std::move(*name);
  }
  switch (vendor_id) {
    case 0x10de:
      return "NVIDIA";
    case 0x1002:
    case 0x1022:
      return "AMD";
    case 0x8086:
      return "Intel";
    default:
      return std::format("{:#06x}", vendor_id);
  }
}

// Version of the user mode driver, e.g. "31.0.15.3623".
std::optional<std::string> driver_version(IDXGIAdapter1* adapter) {
  LARGE_INTEGER version{};
  if (FAILED(adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &version))) {
    return std::nullopt;
  }
  const auto high = static_cast<std::uint32_t>(version.HighPart);
  const auto low = static_cast<std::uint32_t>(version.LowPart);
  return std::format("{}.{}.{}.{}", high >> 16, high & 0xffff, low >> 16, low & 0xffff);
}

#ifdef USE_OCL
void add_opencl_info(std::vector<Gpu>& gpus) {
  for (auto* cl_gpu : opencl_::DeviceManager::get_list<opencl_::Filter::GPU>()) {
    for (auto& gpu : gpus) {
      if (!cl_gpu->name().contains(gpu.name)) {
        continue;
      }
      if (!gpu.driver_version) {
        gpu.driver_version = cl_gpu->driver_version();
      }
      gpu.frequency = cl_gpu->clock_frequency_MHz() * FrequencyUnit::MHz;
      gpu.cores = static_cast<std::uint32_t>(cl_gpu->cores());
      if (!gpu.dedicated_memory) {
        gpu.dedicated_memory = Bytes{cl_gpu->memory_Bytes()};
      }
    }
  }
}
#endif

}  // namespace

result<std::vector<Gpu>> gpus() {
  internal::ComPtr<IDXGIFactory1> factory;
  if (const HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), factory.put_void()); FAILED(hr)) {
    return std::unexpected(internal::hresult_error(hr, "CreateDXGIFactory1"));
  }

  std::vector<Gpu> result;
  for (UINT i = 0;; ++i) {
    internal::ComPtr<IDXGIAdapter1> adapter;
    const HRESULT hr = factory->EnumAdapters1(i, adapter.put());
    if (hr == DXGI_ERROR_NOT_FOUND) {
      break;
    }
    if (FAILED(hr)) {
      return std::unexpected(internal::hresult_error(hr, "IDXGIFactory1::EnumAdapters1"));
    }
    DXGI_ADAPTER_DESC1 desc{};
    if (FAILED(adapter->GetDesc1(&desc)) || (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) != 0) {
      continue;  // e.g. "Microsoft Basic Render Driver"
    }
    const auto vendor_id = static_cast<std::uint16_t>(desc.VendorId);
    const auto device_id = static_cast<std::uint16_t>(desc.DeviceId);
    result.push_back(Gpu{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = vendor_name(vendor_id),
        .name = internal::to_utf8(desc.Description),
        .driver = std::nullopt,
        .driver_version = driver_version(adapter.get()),
        .dedicated_memory = Bytes{desc.DedicatedVideoMemory},
        .shared_memory = Bytes{desc.SharedSystemMemory},
        .frequency = std::nullopt,
        .cores = std::nullopt,
        .pci = PciId{vendor_id, device_id},
    });
  }

#ifdef USE_OCL
  add_opencl_info(result);
#endif
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
