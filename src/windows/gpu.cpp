// Copyright Leon Freist
// Author Leon Freist <freist@informatik.uni-freiburg.de>

#include <hwinfo/platform.h>

#ifdef HWINFO_WINDOWS

// clang-format off
#include <windows.h>
#include <d3dkmthk.h>
#include <dxgi1_6.h>
// clang-format on

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

#include "hwinfo/gpu.h"
#include "hwinfo/utils/stringutils.h"
#include "hwinfo/utils/wmi_wrapper.h"

#ifdef USE_OCL
#include <hwinfo/opencl/device.h>
#endif

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::vector<GPU> getAllGPUs() {
  std::vector<GPU> gpus;

  try {
    utils::WMI::_WMI wmi;
    if (!wmi.execute_query(
            L"SELECT Name, AdapterCompatibility, AdapterRAM, DriverVersion, PNPDeviceID FROM Win32_VideoController")) {
      return gpus;
    }

    ULONG u_return = 0;
    IWbemClassObject* object = nullptr;
    while (wmi.enumerator) {
      wmi.enumerator->Next(static_cast<long>(WBEM_INFINITE), 1, &object, &u_return);
      if (!u_return) {
        break;
      }

      auto readString = [object](const wchar_t* property) {
        VARIANT value;
        VariantInit(&value);
        std::string result;
        if (SUCCEEDED(object->Get(property, 0, &value, nullptr, nullptr)) && V_VT(&value) == VT_BSTR) {
          result = utils::wstring_to_std_string(value.bstrVal);
        }
        VariantClear(&value);
        return result;
      };

      GPU gpu;
      gpu._id = static_cast<std::uint32_t>(gpus.size());
      gpu._name = readString(L"Name");
      gpu._driverVersion = readString(L"DriverVersion");
      gpu._vendor = readString(L"AdapterCompatibility");

      std::string pnpDeviceId = readString(L"PNPDeviceID");
      std::transform(pnpDeviceId.begin(), pnpDeviceId.end(), pnpDeviceId.begin(),
                     [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
      auto readPciId = [&pnpDeviceId](const std::string& prefix) {
        const auto position = pnpDeviceId.find(prefix);
        if (position == std::string::npos || position + prefix.size() + 4 > pnpDeviceId.size()) {
          return std::string{};
        }
        return std::string("0x") + pnpDeviceId.substr(position + prefix.size(), 4);
      };
      gpu._vendor_id = readPciId("VEN_");
      gpu._device_id = readPciId("DEV_");

      VARIANT adapterRam;
      VariantInit(&adapterRam);
      if (SUCCEEDED(object->Get(L"AdapterRAM", 0, &adapterRam, nullptr, nullptr))) {
        if (V_VT(&adapterRam) == VT_UI4) {
          gpu._dedicated_memory_Bytes = V_UI4(&adapterRam);
        } else if (V_VT(&adapterRam) == VT_I4) {
          gpu._dedicated_memory_Bytes = static_cast<std::uint32_t>(V_I4(&adapterRam));
        }
      }
      VariantClear(&adapterRam);

      if (gpu._vendor_id == "0x10DE") {
        gpu._vendor = "NVIDIA";
      } else if (gpu._vendor_id == "0x1002" || gpu._vendor_id == "0x1022") {
        gpu._vendor = "AMD";
      } else if (gpu._vendor_id == "0x8086") {
        gpu._vendor = "Intel";
      }

      gpus.push_back(std::move(gpu));
      object->Release();
    }
  } catch (...) {
  }

  IDXGIFactory1* factory = nullptr;
  if (SUCCEEDED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&factory)))) {
    IDXGIAdapter1* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 desc = {};
      adapter->GetDesc1(&desc);

      if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
        const std::string adapterName = utils::wstring_to_std_string(desc.Description);
        char vendorId[10];
        char deviceId[10];
        sprintf_s(vendorId, "0x%04X", desc.VendorId);
        sprintf_s(deviceId, "0x%04X", desc.DeviceId);

        auto gpu = std::find_if(gpus.begin(), gpus.end(),
                                [&](const GPU& candidate) { return candidate._name == adapterName; });
        if (gpu == gpus.end() && vendorId[0] != '\0' && deviceId[0] != '\0') {
          gpu = std::find_if(gpus.begin(), gpus.end(), [&](const GPU& candidate) {
            return candidate._vendor_id == vendorId && candidate._device_id == deviceId;
          });
        }
        if (gpu != gpus.end()) {
          D3DKMT_OPENADAPTERFROMLUID openAdapter = {};
          openAdapter.AdapterLuid = desc.AdapterLuid;
          if (D3DKMTOpenAdapterFromLuid(&openAdapter) >= 0) {
            D3DKMT_SEGMENTSIZEINFO memoryInfo = {};
            D3DKMT_QUERYADAPTERINFO queryInfo = {};
            queryInfo.hAdapter = openAdapter.hAdapter;
            queryInfo.Type = KMTQAITYPE_GETSEGMENTSIZE;
            queryInfo.pPrivateDriverData = &memoryInfo;
            queryInfo.PrivateDriverDataSize = static_cast<UINT>(sizeof(memoryInfo));

            if (D3DKMTQueryAdapterInfo(&queryInfo) >= 0) {
              gpu->_dedicated_memory_Bytes = memoryInfo.DedicatedVideoMemorySize;
              gpu->_shared_memory_Bytes = memoryInfo.SharedSystemMemorySize;
            }

#if defined(DXGKDDI_INTERFACE_VERSION_WDDM2_4) && defined(DXGKDDI_INTERFACE_VERSION) && \
    DXGKDDI_INTERFACE_VERSION >= DXGKDDI_INTERFACE_VERSION_WDDM2_4
            D3DKMT_NODE_PERFDATA performanceInfo = {};
            performanceInfo.NodeOrdinal = 0;
            performanceInfo.PhysicalAdapterIndex = 0;
            queryInfo.Type = KMTQAITYPE_NODEPERFDATA;
            queryInfo.pPrivateDriverData = &performanceInfo;
            queryInfo.PrivateDriverDataSize = static_cast<UINT>(sizeof(performanceInfo));

            if (D3DKMTQueryAdapterInfo(&queryInfo) >= 0) {
              gpu->_frequency_hz = performanceInfo.MaxFrequency;
            }
#endif

            D3DKMT_CLOSEADAPTER closeAdapter = {};
            closeAdapter.hAdapter = openAdapter.hAdapter;
            D3DKMTCloseAdapter(&closeAdapter);
          }
        }
      }

      adapter->Release();
    }
    factory->Release();
  }

#ifdef USE_OCL
  auto cl_gpus = opencl_::DeviceManager::get_list<opencl_::Filter::GPU>();
  for (auto& gpu : gpus) {
    for (auto* cl_gpu : cl_gpus) {
      if (cl_gpu->name() == gpu._name) {
        gpu._num_cores = cl_gpu->cores();
        if (gpu._frequency_hz == 0) {
          gpu._frequency_hz = cl_gpu->clock_frequency_MHz() * 1'000'000;
        }
        break;
      }
    }
  }
#endif
  return gpus;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS
