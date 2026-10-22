// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <d3d12.h>
#include <dxgi.h>
#ifdef _MSC_VER
#include <d3dkmthk.h>
#endif
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
#include "internal/windows_gpu.h"
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
  switch (vendor_id) {
    case 0x10de:
      return "NVIDIA";
    case 0x1002:
    case 0x1022:
      return "AMD";
    case 0x8086:
      return "Intel";
    default:
      return internal::lookup_pci(pci_database(), static_cast<std::uint16_t>(vendor_id), 0).vendor.value_or("");
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

// Adapter information via the D3DKMT thunks of gdi32. Declared here because d3dkmthk.h is missing from
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
struct SegmentSize {
  alignas(8) std::uint64_t dedicated_video;
  alignas(8) std::uint64_t dedicated_system;
  alignas(8) std::uint64_t shared_system;
};
struct NodePerfData {
  UINT node;
  UINT physical_adapter;
  alignas(8) std::uint64_t frequency;
  alignas(8) std::uint64_t max_frequency;
  alignas(8) std::uint64_t max_frequency_oc;
  ULONG voltage;
  ULONG max_voltage;
  ULONG max_voltage_oc;
  alignas(8) std::uint64_t max_transition_latency;
};
constexpr UINT query_segment_size = 3;     // KMTQAITYPE_GETSEGMENTSIZE
constexpr UINT query_adapter_address = 6;  // KMTQAITYPE_ADAPTERADDRESS
constexpr UINT query_node_perf_data = 61;  // KMTQAITYPE_NODEPERFDATA (WDDM 2.4+)
#ifdef _MSC_VER
static_assert(sizeof(OpenAdapterFromLuid) == sizeof(D3DKMT_OPENADAPTERFROMLUID));
static_assert(sizeof(QueryAdapterInfo) == sizeof(D3DKMT_QUERYADAPTERINFO));
static_assert(sizeof(CloseAdapter) == sizeof(D3DKMT_CLOSEADAPTER));
static_assert(sizeof(AdapterAddress) == sizeof(D3DKMT_ADAPTERADDRESS));
static_assert(sizeof(SegmentSize) == sizeof(D3DKMT_SEGMENTSIZEINFO));
#if defined(DXGKDDI_INTERFACE_VERSION_WDDM2_4) && DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4
static_assert(sizeof(NodePerfData) == sizeof(D3DKMT_NODE_PERFDATA));
#endif
#endif
}  // namespace d3dkmt

void adapter_info(Gpu& gpu, const LUID& luid) {
  using Open = LONG(APIENTRY*)(d3dkmt::OpenAdapterFromLuid*);
  using Query = LONG(APIENTRY*)(const d3dkmt::QueryAdapterInfo*);
  using Close = LONG(APIENTRY*)(const d3dkmt::CloseAdapter*);
  static const HMODULE gdi = LoadLibraryW(L"gdi32.dll");
  if (gdi == nullptr) {
    return;
  }
  const auto open = reinterpret_cast<Open>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTOpenAdapterFromLuid")));
  const auto query = reinterpret_cast<Query>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTQueryAdapterInfo")));
  const auto close = reinterpret_cast<Close>(reinterpret_cast<void*>(GetProcAddress(gdi, "D3DKMTCloseAdapter")));
  if (open == nullptr || query == nullptr || close == nullptr) {
    return;
  }
  d3dkmt::OpenAdapterFromLuid adapter{.adapter_luid = luid, .adapter = 0};
  if (open(&adapter) < 0) {
    return;
  }
  d3dkmt::AdapterAddress address{};
  d3dkmt::QueryAdapterInfo info{
      .adapter = adapter.adapter,
      .type = d3dkmt::query_adapter_address,
      .data = &address,
      .data_size = sizeof(address),
  };
  if (query(&info) >= 0 && gpu.pci && address.bus <= 0xff && address.device <= 0x1f && address.function <= 0x7) {
    gpu.pci->address = internal::format_pci_address(0, address.bus, address.device, address.function);
  }
  // Unlike DXGI's SIZE_T fields, these sizes remain 64 bit in a 32 bit process.
  d3dkmt::SegmentSize memory{};
  info.type = d3dkmt::query_segment_size;
  info.data = &memory;
  info.data_size = sizeof(memory);
  if (query(&info) >= 0) {
    gpu.dedicated_memory = Bytes{memory.dedicated_video};
    gpu.shared_memory = Bytes{memory.shared_system};
  }
  d3dkmt::NodePerfData performance{};
  info.type = d3dkmt::query_node_perf_data;
  info.data = &performance;
  info.data_size = sizeof(performance);
  if (query(&info) >= 0 && performance.max_frequency != 0) {
    gpu.max_frequency = Hertz{performance.max_frequency};
  }
  const d3dkmt::CloseAdapter close_info{.adapter = adapter.adapter};
  close(&close_info);
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
  const auto controllers = internal::wmi::query(
      "Win32_VideoController", {"Name", "AdapterCompatibility", "AdapterRAM", "DriverVersion", "PNPDeviceID"});
  if (!controllers) {
    return std::unexpected(controllers.error());
  }
  std::vector<Gpu> result;
  for (const auto& row : *controllers) {
    auto gpu = internal::video_controller(row);
    if (gpu.pci) {
      if (auto vendor = vendor_name(gpu.pci->vendor_id); !vendor.empty()) {
        gpu.vendor = std::move(vendor);
      }
    }
    result.push_back(std::move(gpu));
  }

  // WMI supplies the device list even when DXGI is unavailable to the calling process.
  internal::ComPtr<IDXGIFactory1> factory;
  const HRESULT factory_hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), factory.put_void());
  for (UINT i = 0; SUCCEEDED(factory_hr); ++i) {
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
      };
    }
    Gpu gpu{
        .vendor = vendor_name(desc.VendorId),
        .name = internal::to_utf8(desc.Description),
        .luid = static_cast<std::uint64_t>(static_cast<std::uint32_t>(desc.AdapterLuid.HighPart)) << 32 |
                desc.AdapterLuid.LowPart,
        .driver_version = driver_version(adapter.get()),
        .pci = std::move(pci),
    };
    if constexpr (sizeof(SIZE_T) >= sizeof(std::uint64_t)) {
      gpu.dedicated_memory = Bytes{desc.DedicatedVideoMemory};
      gpu.shared_memory = Bytes{desc.SharedSystemMemory};
    }
    adapter_info(gpu, desc.AdapterLuid);
    gpu.type = classify(adapter.get(), gpu.pci);
    gpu.unified_memory = internal::unified_memory(gpu.type);
    const auto controller = internal::match_video_controller(result, gpu);
    if (controller == result.end()) {
      result.push_back(std::move(gpu));
    } else {
      if (gpu.vendor.empty()) {
        gpu.vendor = std::move(controller->vendor);
      }
      if (!controller->name.empty()) {
        gpu.name = std::move(controller->name);
      }
      if (controller->driver_version) {
        gpu.driver_version = std::move(controller->driver_version);
      }
      if (!gpu.dedicated_memory) {
        gpu.dedicated_memory = controller->dedicated_memory;
      }
      *controller = std::move(gpu);
    }
  }

  internal::gpu::enrich(result, query, internal::gpu::OsPriority::highest);
  internal::order_gpus(result);
  return result;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
