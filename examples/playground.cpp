#include <windows.h>
#include <dxgi1_6.h>
#include <setupapi.h>
#include <devguid.h>
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "setupapi.lib")

#include <iostream>

void getAllGPUs() {
  IDXGIFactory1* pFactory;
  if (FAILED(CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory))) return;

  IDXGIAdapter1* pAdapter;
  for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
    DXGI_ADAPTER_DESC1 desc;
    pAdapter->GetDesc1(&desc);

    // Skip software renderers (like Basic Render Driver)
    if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) {
      pAdapter->Release();
      continue;
    }

    std::cout << "id: " << static_cast<int>(i) << std::endl;

    // Convert WCHAR name to std::string
    std::wstring ws(desc.Description);
    std::cout << "name: " << std::string(ws.begin(), ws.end()) << std::endl;

    std::cout << "memory: " << static_cast<int64_t>(desc.DedicatedVideoMemory) << std::endl;

    // Hex formatting for IDs
    char buffer[10];
    sprintf_s(buffer, "0x%04X", desc.VendorId);
    std::cout << "vendor: " << buffer << std::endl;
    sprintf_s(buffer, "0x%04X", desc.DeviceId);
    std::cout << "device_id: " << buffer << std::endl;

    // Resolve Vendor Name
    if (desc.VendorId == 0x10DE) {
      std::cout << "NVIDIA" << std::endl;
    }
    else if (desc.VendorId == 0x1002 || desc.VendorId == 0x1022) {
      std::cout << "AMD" << std::endl;
    }
    else if (desc.VendorId == 0x8086) {
      std::cout << "Intel" << std::endl;
    }
    else {
      std::cout << "Unknown" << std::endl;
    }

    // Get Driver Version via SetupAPI
    HDEVINFO hDevInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE) {
      SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };
      for (DWORD j = 0; SetupDiEnumDeviceInfo(hDevInfo, j, &devInfoData); ++j) {
        // In a real impl, you'd match the DeviceID here.
        // For now, we'll grab the driver from the corresponding index.
        WCHAR driverVersion[256];
        if (SetupDiGetDeviceRegistryPropertyW(hDevInfo, &devInfoData, SPDRP_DRIVER, nullptr, (PBYTE)driverVersion, sizeof(driverVersion), nullptr)) {
          // This gives the registry key, we then query the 'DriverVersion' value
          // (Simplified for brevity)
        }
      }
      SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    pAdapter->Release();
  }
  pFactory->Release();
}

int main() {
  getAllGPUs();
}