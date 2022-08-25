// Copyright (c) Leon Freist <freist@informatik.uni-freiburg.de>
// This software is part of HWBenchmark

#include <sstream>
#include <string>

#if defined(unix) || defined(__unix) || defined(__unix__)
#include <fstream>
#include <sys/stat.h>
#include <sys/utsname.h>
#elif defined(__APPLE__)
#include <sys/sysctl.h>
#elif defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)
#include <Windows.h>
#include <winternl.h>
#define STATUS_SUCCESS 0x00000000
#endif

#include "hwinfo/os.h"

using std::string;

namespace hwinfo {

// _____________________________________________________________________________________________________________________
OS::OS() {
  _fullName = getFullName();
  _name = getName();
  _version = getVersion();
  _kernel = getKernel();
  _32bit = getIs32bit();
  _64bit = getIs64bit();
  _bigEndian = getIsBigEndian();
  _littleEndian = getIsLittleEndian();
}

// _____________________________________________________________________________________________________________________
OS::~OS() = default;

// _____________________________________________________________________________________________________________________
string OS::fullName() {
  return _fullName;
}

// _____________________________________________________________________________________________________________________
string OS::name() {
  return _name;
}

// _____________________________________________________________________________________________________________________
string OS::version() {
  return _version;
}

// _____________________________________________________________________________________________________________________
string OS::kernel() {
  return _kernel;
}

// _____________________________________________________________________________________________________________________
bool OS::is32bit() const {
  return _32bit;
}

// _____________________________________________________________________________________________________________________
bool OS::is64bit() const {
  return _64bit;
}

// _____________________________________________________________________________________________________________________
bool OS::isBigEndian() const {
  return _bigEndian;
}

// _____________________________________________________________________________________________________________________
bool OS::isLittleEndian() const {
  return _littleEndian;
}

// _____________________________________________________________________________________________________________________
string OS::getFullName() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  string line;
  std::ifstream stream("/etc/lsb-release");
  if (!stream) {
    return "Linux <unknown version>";
  }
  while (getline(stream, line)) {
    if (line.starts_with("DISTRIB_DESCRIPTION")) {
      return line.substr(line.find('=')+1, line.length());
    }
  }
  stream.close();
  return "Linux <unknown version>";
#elif defined(__APPLE__)
  string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.os", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "macOS <unknown version>";
#elif defined(_WIN32) || defined(_WIN64)
  static NTSTATUS(__stdcall *RtlGetVersion)(OUT PRTL_OSVERSIONINFOEXW lpVersionInformation) = (NTSTATUS(__stdcall*)(PRTL_OSVERSIONINFOEXW))GetProcAddress(GetModuleHandle("ntdll.dll"), "RtlGetVersion");
  static void(__stdcall *GetNativeSystemInfo)(OUT LPSYSTEM_INFO lpSystemInfo) = (void(__stdcall*)(LPSYSTEM_INFO))GetProcAddress(GetModuleHandle("kernel32.dll"), "GetNativeSystemInfo");
  static BOOL(__stdcall *GetProductInfo)(IN DWORD dwOSMajorVersion, IN DWORD dwOSMinorVersion, IN DWORD dwSpMajorVersion, IN DWORD dwSpMinorVersion, OUT PDWORD pdwReturnedProductType) = (BOOL(__stdcall*)(DWORD, DWORD, DWORD, DWORD, PDWORD))GetProcAddress(GetModuleHandle("kernel32.dll"), "GetProductInfo");

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
#pragma warning(disable: 4996) // C4996: 'function': was declared deprecated
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
        }
        else {
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
    } if (osvi.wProductType != VER_NT_WORKSTATION) {
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
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
string OS::getName() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  string line;
  std::ifstream stream("/etc/lsb-release");
  if (!stream) {
    return "Linux";
  }
  while (getline(stream, line)) {
    if (line.starts_with("DISTRIB_ID")) {
      return line.substr(line.find('=')+1, line.length());
    }
  }
  stream.close();
  return "Linux";
#elif defined(__APPLE__)
  string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.os", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "macOS";
#elif defined(_WIN32) || defined(_WIN64)
  return "Windows";
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
string OS::getVersion() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  string line;
  std::ifstream stream("/etc/lsb-release");
  if (!stream) {
    return "<unknown version>";
  }
  while (getline(stream, line)) {
    if (line.starts_with("DISTRIB_RELEASE")) {
      return line.substr(line.find('=')+1, line.length());
    }
  }
  stream.close();
  return "<unknown version>";
#elif defined(__APPLE__)
  string os_name;
  os_name.resize(1024);
  size_t size = sizeof(os_name);
  if (sysctlbyname("kern.osrelease", os_name.data(), &size, nullptr, 0) == 0) {
    return os_name;
  }
  return "<unknown version>";
#elif defined(_WIN32) || defined(_WIN64)
  return "<undefined>";
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
string OS::getKernel() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  static utsname info;
  if (uname(&info) == 0) {
    return info.release;
  }
  return "<unknown kernel>";
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
bool OS::getIs32bit() {
  return !getIs64bit();
}

// _____________________________________________________________________________________________________________________
bool OS::getIs64bit() {
#if defined(unix) || defined(__unix) || defined(__unix__)
  struct stat buffer{};
  return (stat("/lib64/ld-linux-x86-64.so.2", &buffer) == 0);
#elif defined(__APPLE__)
  return true;
#elif defined(_WIN32) || defined(__CYGWIN__)
  BOOL bWow64Process = FALSE;
  return IsWow64Process(GetCurrentProcess(), &bWow64Process) && bWow64Process;
#elif defined(_WIN64)
  return true;
#else
  return "<unknown>";
#endif
}

// _____________________________________________________________________________________________________________________
bool OS::getIsBigEndian() {
  char16_t dummy = 0x0102;
  return ((char*)&dummy)[0] == 0x01;
}

// _____________________________________________________________________________________________________________________
bool OS::getIsLittleEndian() {
  char16_t dummy = 0x0102;
  return ((char*)&dummy)[0] == 0x02;
}

}  // namespace hwinfo