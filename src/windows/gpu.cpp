// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <d3d12.h>
#include <dxgi.h>
// clang-format on
#include <hwinfo/gpu.h>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal/gpu.h"
#include "internal/gpu_backend.h"
#include "internal/pci.h"
#include "internal/windows_com.h"
#include "internal/windows_error.h"
#include "internal/windows_strings.h"
#include "pci.ids.h"

#ifdef _MSC_VER
#pragma comment(lib, "dxgi.lib")
#endif

namespace hwinfo {

namespace {

std::string_view pci_database() { return {reinterpret_cast<const char*>(pci_ids), pci_ids_size}; }

// DXGI reports PCI vendor ids, or ACPI vendor ids ("QCOM") for GPUs that are not PCI devices (Windows on ARM).
std::string vendor_name(UINT vendor_id) {
  if (vendor_id > 0xffff) {
    std::string acpi;
    for (int shift = 0; shift < 32; shift += 8) {
      acpi.push_back(static_cast<char>((vendor_id >> shift) & 0xff));
    }
    return acpi == "QCOM" ? "Qualcomm" : acpi;
  }
  if (auto name = internal::lookup_pci(pci_database(), static_cast<std::uint16_t>(vendor_id), 0).vendor) {
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

// PCI location of the adapter via the D3DKMT thunks of gdi32. Declared here because d3dkmthk.h is missing from
// MinGW; the structures are part of the stable WDDM user mode ABI.
namespace d3dkmt {
using Handle = UINT;
struct OpenAdapterFromLuid {
  LUID adapter_luid;
  Handle adapter;
};
struct QueryAdapterInfo {
  Handle adapter;
  UINT type;
  void* data;
  UINT data_size;
};
struct CloseAdapter {
  Handle adapter;
};
struct AdapterAddress {
  UINT bus;
  UINT device;
  UINT function;
};
constexpr UINT query_adapter_address = 6;  // KMTQAITYPE_ADAPTERADDRESS
}  // namespace d3dkmt

std::optional<std::string> pci_address(const LUID& luid) {
  using Open = LONG(APIENTRY*)(d3dkmt::OpenAdapterFromLuid*);
  using Query = LONG(APIENTRY*)(const d3dkmt::QueryAdapterInfo*);
  using Close = LONG(APIENTRY*)(const d3dkmt::CloseAdapter*);
  static const HMODULE gdi = LoadLibraryW(L"gdi32.dll");
  if (gdi == nullptr) {
    return std::nullopt;
  }
  const auto open = reinterpret_cast<Open>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid")));
  const auto query = reinterpret_cast<Query>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTQueryAdapterInfo")));
  const auto close = reinterpret_cast<Close>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTCloseAdapter")));
  if (open == nullptr || query == nullptr || close == nullptr) {
    return std::nullopt;
  }
  d3dkmt::OpenAdapterFromLuid adapter{.adapter_luid = luid, .adapter = 0};
  if (open(&adapter) != 0) {
    return std::nullopt;
  }
  d3dkmt::AdapterAddress address{};
  const d3dkmt::QueryAdapterInfo info{
      .adapter = adapter.adapter,
      .type = d3dkmt::query_adapter_address,
      .data = &address,
      .data_size = sizeof(address),
  };
  const LONG status = query(&info);
  const d3dkmt::CloseAdapter close_info{.adapter = adapter.adapter};
  close(&close_info);
  if (status != 0) {
    return std::nullopt;
  }
  return internal::format_pci_address(0, address.bus, address.device, address.function);
}

// Whether the adapter has a unified memory architecture (integrated GPU), as reported by a D3D12 device created on it.
// d3d12.dll is loaded at runtime: it is missing before Windows 10 and not needed by anything else.
std::optional<bool> is_uma(IDXGIAdapter1* adapter) {
  static const HMODULE d3d12 = LoadLibraryW(L"d3d12.dll");
  if (d3d12 == nullptr) {
    return std::nullopt;
  }
  static const auto create =
      reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(reinterpret_cast<void*>(GetProcAddress(d3d12, "D3D12CreateDevice")));
  if (create == nullptr) {
    return std::nullopt;
  }
  internal::ComPtr<ID3D12Device> device;
  if (FAILED(create(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), device.put_void()))) {
    return std::nullopt;  // no D3D12 driver
  }
  D3D12_FEATURE_DATA_ARCHITECTURE architecture{};
  if (FAILED(device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &architecture, sizeof(architecture)))) {
    return std::nullopt;
  }
  return architecture.UMA != FALSE;
}

GpuType classify(IDXGIAdapter1* adapter, const std::optional<PciDevice>& pci) {
  if (pci && internal::is_virtual_gpu_vendor(pci->vendor_id)) {
    return GpuType::virtualized;
  }
  if (const auto uma = is_uma(adapter)) {
    return *uma ? GpuType::integrated : GpuType::discrete;
  }
  return pci ? internal::classify_pci_gpu(*pci) : GpuType::unknown;
}

}  // namespace

result<std::vector<Gpu>> gpus(const GpuQuery& query) {
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
    std::optional<PciDevice> pci;
    if (desc.VendorId <= 0xffff) {
      pci = PciDevice{
          .vendor_id = static_cast<std::uint16_t>(desc.VendorId),
          .device_id = static_cast<std::uint16_t>(desc.DeviceId),
          .address = pci_address(desc.AdapterLuid),
      };
    }
    const GpuType type = classify(adapter.get(), pci);
    result.push_back(Gpu{
        .index = static_cast<std::uint32_t>(result.size()),
        .vendor = vendor_name(desc.VendorId),
        .name = internal::to_utf8(desc.Description),
        .type = type,
        .unified_memory = internal::unified_memory(type),
        .luid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(desc.AdapterLuid.HighPart)) << 32 |
                desc.AdapterLuid.LowPart,
        .driver_version = driver_version(adapter.get()),
        .dedicated_memory = Bytes{desc.DedicatedVideoMemory},
        .shared_memory = Bytes{desc.SharedSystemMemory},
        .pci = std::move(pci),
    });
  }

  internal::gpu::enrich(result, query);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
