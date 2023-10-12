#if defined(_WIN32) || defined(_WIN64) || defined(__CYGWIN__)

#pragma once

#define _WIN32_DCOM
#include <WbemIdl.h>
#include <comdef.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
#pragma comment(lib, "wbemuuid.lib")

class CPU;
class GPU;
class Disk;
class RAM;
class MainBoard;

namespace hwinfo {
namespace utils {
namespace WMI {

template <typename T>
std::vector<T> get_component();


template <typename T>
std::vector<T> query(const std::wstring& wmi_class, const std::wstring& field);

}
}
}

#endif