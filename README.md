[![Linux (clang)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml)
[![Linux (gcc)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml)

[![MacOS](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml)

[![Windows (MinGW)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml)
[![Windows (Visual Studio)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml)

# hwinfo

hwinfo provides an easy-to-use and modern C++ API for retrieving hardware information of your systems components such as CPU, RAM, GPU, Disks, Mainboard, ...

## Content

* [Functionality](#functionality)
* [Build hwinfo](#build-hwinfo)
* [Example](#example)
* [Include hwinfo to cmake project](#include-hwinfo-in-your-cmake-project)

## Functionality

| Component        | Info               | Linux | Apple | Windows |
|------------------|:-------------------|:-----:|:-----:|:-------:|
| CPU              | Vendor             |  ✔️   |  ✔️   |   ✔️    |
|                  | Model              |  ✔️   |  ✔️   |   ✔️    |
|                  | Frequency          |  ✔️   |  ✔️   |   ✔️    |
|                  | Physical Cores     |  ✔️   |  ✔️   |   ✔️    |
|                  | Logical Cores      |  ✔️   |  ✔️   |   ✔️    |
|                  | Cache Size         |  ✔️   |  ✔️   |   ✔️    |
| GPU              | Vendor             |  ✔️   |  ✔️   |   ✔️    |
|                  | Model              |  ✔️   |  ✔️   |   ✔️    |
|                  | Memory Size        |   ❌   |   ❌   |   ✔️    |
| Memory (RAM)     | Vendor             |   ❌   |   ❌   |   ✔️    |
|                  | Model              |   ❌   |   ❌   |   ✔️    |
|                  | Name               |   ❌   |   ❌   |   ✔️    |
|                  | Serial Number      |   ❌   |   ❌   |   ✔️    |
|                  | Total Memory Size  |  ✔️   |  ✔️   |   ✔️    |
|                  | Free Memory Size   |   ❌   |   ❌   |    ❌    |
| Mainboard        | Vendor             |  ✔️   |   ❌   |   ✔️    |
|                  | Model              |  ✔️   |   ❌   |   ✔️    |
|                  | Version            |  ✔️   |   ❌   |   ✔️    |
|                  | Serial-Number      |   ❌   |   ❌   |   ✔️    |
|                  | Bios               |   ❌   |   ❌   |    ❌    |
| Disk             | Vendor             |  ✔️   |   ❌   |    ✔    |
|                  | Model              |  ✔️   |   ❌   |    ✔    |
|                  | Serial-Number      |   ❌   |   ❌   |    ✔    |
|                  | Size               |   ❌   |   ❌   |    ❌    |
| Operating System | Name               |  ✔️   |  ✔️   |   ✔️    |
|                  | Short Name         |  ✔️   |  ✔️   |   ✔️    |
|                  | Version            |  ✔️   |  ✔️   |    ❌    |
|                  | Kernel             |  ✔️   |   ❌   |    ❌    |
|                  | Architecture (Bit) |  ✔️   |  ✔️   |   ✔️    |
|                  | Endianess          |  ✔️   |  ✔️   |   ✔️    |

## Build `hwinfo`

> Requirements: git, cmake, c++ compiler (gcc, clang, MSVC)

1. Download repository:
    ```
    git clone https://github.com/lfreist/hwinfo
    ```
2. Build using cmake:
    ```
    mkdir build
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release .. && make -j $(nproc)
    ```

## Example

See [example.cpp](examples/example.cpp)

## Include `hwinfo` in your cmake project

1. Download `hwinfo` into your project (e.g. in `<project-root>/third_party/hwinfo`)
    ```
    mkdir third_party
    cd third_party
    git clone https://github.com/lfreist/hwinfo
    ```
2. Simply add the following to your `<project-root>/CMakeLists.txt` file:
    ```cmake
    # file: <project-root>/CMakeLists.txt
    
    add_subdirectory(third_party/hwinfo)
    include_directories(third_party/hwinfo/include)
    ```
3. Include `hwinfo` into your `.cpp/.h` files:
    ```c++
    // file: your_benchmarks.cpp
    
    #include "hwinfo/hwinfo.h"
    ```
4. Link it in cmake
    ```cmake
    add_executable(your_executable your_executable.cpp)
    target_link_libraries(your_executable PUBLIC hwinfo::HWinfo)
    ```
