// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include "hwinfo/platform.h"

#ifdef HWINFO_WINDOWS

#include <Windows.h>
#include <winternl.h>

#include <sstream>
#include <string>
#define STATUS_SUCCESS 0x00000000

#include "hwinfo/os.h"

namespace hwinfo {

// _____________________________________________________________________________________________________________________
std::string OS::getFullName() {
  static NTSTATUS(__stdcall * RtlGetVersion)(OUT PRTL_OSVERSIONINFOEXW lpVersionInformation) =
      (NTSTATUS(__stdcall*)(PRTL_OSVERSIONINFOEXW))GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlGetVersion");
  static void(__stdcall * GetNativeSystemInfo)(OUT LPSYSTEM_INFO lpSystemInfo) =
      (void(__stdcall*)(LPSYSTEM_INFO))GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
  static BOOL(__stdcall * GetProductInfo)(IN DWORD dwOSMajorVersion, IN DWORD dwOSMinorVersion,
                                          IN DWORD dwSpMajorVersion, IN DWORD dwSpMinorVersion,
                                          OUT PDWORD pdwReturnedProductType) =
      (BOOL(__stdcall*)(DWORD, DWORD, DWORD, DWORD, PDWORD))GetProcAddress(GetModuleHandle("kernel32.dll"),
                                                                           "GetProductInfo");

  OSVERSIONINFOEXW osvi;
  ZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);

  if (RtlGetVersion != nullptr) {
    NTSTATUS ntRtlGetVersionStatus = RtlGetVersion(&osvi);
    if (ntRtlGetVersionStatus != STATUS_SUCCESS) {
      return "<windows>";
    }
  } else {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)  // C4996: 'function': was declared deprecated
#endif
    BOOL bOsVersionInfoEx = GetVersionExW((OSVERSIONINFOW*)&osvi);
    if (bOsVersionInfoEx == 0) {
      return "<windows>";
    }
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  }

  SYSTEM_INFO si;
  ZeroMemory(&si, sizeof(SYSTEM_INFO));

  if (GetNativeSystemInfo != nullptr) {
    GetNativeSystemInfo(&si);
  } else {
    GetSystemInfo(&si);
  }

  if ((osvi.dwPlatformId != VER_PLATFORM_WIN32_NT) || (osvi.dwMajorVersion <= 4)) {
    return "Windows <unknown version>";
  }

  std::stringstream os;
  os << "Microsoft ";
  if (osvi.dwMajorVersion >= 6) {
    if (osvi.dwMajorVersion == 10) {
      if (osvi.dwMinorVersion == 0) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          if (osvi.dwBuildNumber >= 22000) {
            os << "Windows 11 ";
          } else {
            os << "Windows 10 ";
          }
        } else {
          if (osvi.dwBuildNumber >= 20348) {
            os << "Windows Server 2022 ";
          } else if (osvi.dwBuildNumber >= 17763) {
            os << "Windows Server 2019 ";
          } else {
            os << "Windows Server 2016 ";
          }
        }
      }
    } else if (osvi.dwMajorVersion == 6) {
      if (osvi.dwMinorVersion == 3) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          os << "Windows 8.1 ";
        } else {
          os << "Windows Server 2012 R2 ";
        }
      } else if (osvi.dwMinorVersion == 2) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          os << "Windows 8 ";
        } else {
          os << "Windows Server 2012 ";
        }
      } else if (osvi.dwMinorVersion == 1) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          os << "Windows 7 ";
        } else {
          os << "Windows Server 2008 R2 ";
        }
      } else if (osvi.dwMinorVersion == 0) {
        if (osvi.wProductType == VER_NT_WORKSTATION) {
          os << "Windows Vista ";
        } else {
          os << "Windows Server 2008 ";
        }
      }
    }

    DWORD dwType;
    if ((GetProductInfo != nullptr) && GetProductInfo(osvi.dwMajorVersion, osvi.dwMinorVersion, 0, 0, &dwType)) {
      switch (dwType) {
        case PRODUCT_ULTIMATE:
          os << "Ultimate Edition";
          break;
        case PRODUCT_PROFESSIONAL:
          os << "Professional";
          break;
        case PRODUCT_HOME_PREMIUM:
          os << "Home Premium Edition";
          break;
        case PRODUCT_HOME_BASIC:
          os << "Home Basic Edition";
          break;
        case PRODUCT_ENTERPRISE:
          os << "Enterprise Edition";
          break;
        case PRODUCT_BUSINESS:
          os << "Business Edition";
          break;
        case PRODUCT_STARTER:
          os << "Starter Edition";
          break;
        case PRODUCT_CLUSTER_SERVER:
          os << "Cluster Server Edition";
          break;
        case PRODUCT_DATACENTER_SERVER:
          os << "Datacenter Edition";
          break;
        case PRODUCT_DATACENTER_SERVER_CORE:
          os << "Datacenter Edition (core installation)";
          break;
        case PRODUCT_ENTERPRISE_SERVER:
          os << "Enterprise Edition";
          break;
        case PRODUCT_ENTERPRISE_SERVER_CORE:
          os << "Enterprise Edition (core installation)";
          break;
        case PRODUCT_ENTERPRISE_SERVER_IA64:
          os << "Enterprise Edition for Itanium-based Systems";
          break;
        case PRODUCT_SMALLBUSINESS_SERVER:
          os << "Small Business Server";
          break;
        case PRODUCT_SMALLBUSINESS_SERVER_PREMIUM:
          os << "Small Business Server Premium Edition";
          break;
        case PRODUCT_STANDARD_SERVER:
          os << "Standard Edition";
          break;
        case PRODUCT_STANDARD_SERVER_CORE:
          os << "Standard Edition (core installation)";
          break;
        case PRODUCT_WEB_SERVER:
          os << "Web Server Edition";
          break;
        default:
          break;
      }
    }
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 2)) {
    if (GetSystemMetrics(SM_SERVERR2)) {
      os << "Windows Server 2003 R2, ";
    } else if (osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER) {
      os << "Windows Storage Server 2003";
    } else if (osvi.wSuiteMask & VER_SUITE_WH_SERVER) {
      os << "Windows Home Server";
    } else if ((osvi.wProductType == VER_NT_WORKSTATION) &&
               (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)) {
      os << "Windows XP Professional x64 Edition";
    } else {
      os << "Windows Server 2003, ";
    }
    if (osvi.wProductType != VER_NT_WORKSTATION) {
      if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
          os << "Datacenter Edition for Itanium-based Systems";
        } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
          os << "Enterprise Edition for Itanium-based Systems";
        }
      } else if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) {
        if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
          os << "Datacenter x64 Edition";
        } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
          os << "Enterprise x64 Edition";
        } else {
          os << "Standard x64 Edition";
        }
      } else {
        if (osvi.wSuiteMask & VER_SUITE_COMPUTE_SERVER) {
          os << "Compute Cluster Edition";
        } else if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
          os << "Datacenter Edition";
        } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
          os << "Enterprise Edition";
        } else if (osvi.wSuiteMask & VER_SUITE_BLADE) {
          os << "Web Edition";
        } else {
          os << "Standard Edition";
        }
      }
    }
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 1)) {
    os << "Windows XP ";
    if (osvi.wSuiteMask & VER_SUITE_PERSONAL) {
      os << "Home Edition";
    } else {
      os << "Professional";
    }
  } else if ((osvi.dwMajorVersion == 5) && (osvi.dwMinorVersion == 0)) {
    os << "Windows 2000 ";
    if (osvi.wProductType == VER_NT_WORKSTATION) {
      os << "Professional";
    } else {
      if (osvi.wSuiteMask & VER_SUITE_DATACENTER) {
        os << "Datacenter Server";
      } else if (osvi.wSuiteMask & VER_SUITE_ENTERPRISE) {
        os << "Advanced Server";
      } else {
        os << "Server";
      }
    }
  }
  os << " (build " << osvi.dwBuildNumber << ")";
  return os.str();
}

// _____________________________________________________________________________________________________________________
std::string OS::getName() { return "Windows"; }

// _____________________________________________________________________________________________________________________
std::string OS::getVersion() { return "<unknown>"; }

// _____________________________________________________________________________________________________________________
std::string OS::getKernel() { return "<unknown>"; }

// _____________________________________________________________________________________________________________________
bool OS::getIs64bit() {
  BOOL isWow64 = FALSE;
  IsWow64Process(GetCurrentProcess(), &isWow64);

  if (sizeof(void*) == 8 || isWow64)
    return true;
  else
    return false;
}

}  // namespace hwinfo

#endif  // HWINFO_WINDOWS