[![Linux (clang)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml)
[![Linux (gcc)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml)

[![MacOS](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml)

[![Windows (MinGW)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml)
[![Windows (Visual Studio)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml)

# hwinfo

hwinfo provides an easy-to-use and modern C++ API for retrieving hardware information of your systems components such as CPU, RAM, GPU, Disks, Mainboard, ...

> **Note**
>
> If you face any issues, find bugs or if your platform is not supported yet, do not hesitate to [create an issue](https://github.com/lfreist/hwinfo/issues).

## Content

* [Supported Components](#supported-components)
* [Build hwinfo](#build-hwinfo)
* [Example](#example)
* [Include hwinfo to cmake project](#include-hwinfo-in-your-cmake-project)

## Supported Components
> **Note**
>
> The listed components that are not yet implemented (indicated with ❌) are in development and will be supported in
> later releases. **You are welcome to start contributing and help improving this library!**

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
| Disk             | Vendor             |  ✔️   |   ❌   |   ✔️    |
|                  | Model              |  ✔️   |   ❌   |   ✔️    |
|                  | Serial-Number      |   ❌   |   ❌   |   ✔️    |
|                  | Size               |   ❌   |   ❌   |    ❌    |
| Operating System | Name               |  ✔️   |  ✔️   |   ✔️    |
|                  | Short Name         |  ✔️   |  ✔️   |   ✔️    |
|                  | Version            |  ✔️   |  ✔️   |    ❌    |
|                  | Kernel             |  ✔️   |   ❌   |    ❌    |
|                  | Architecture (Bit) |  ✔️   |  ✔️   |   ✔️    |
|                  | Endianess          |  ✔️   |  ✔️   |   ✔️    |
| Battery          | Vendor             |  ✔️   |  ❌️   |   ❌️    |
|                  | Model              |  ✔️   |   ❌   |    ❌    |
|                  | Serial Number      |  ✔️   |   ❌   |    ❌    |
|                  | Technology         |  ✔️   |   ❌   |    ❌    |
|                  | Capacity           |  ✔️   |   ❌   |   ❌️    |
|                  | Charging           |  ✔️   |   ❌   |   ❌️    |

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

The output should look similar to this one:

```
Hardware Report:

----------------------------------- CPU -----------------------------------
vendor:             GenuineIntel
model:              Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz
physical cores:     8
logical cores:      16
max frequency:      5100000
regular frequency:  3800000
current frequency:  4700189
cache size:         16384000
----------------------------------- OS ------------------------------------
Operating System:   Ubuntu 22.04 LTS
short name:         Ubuntu
version:            22.04
kernel:             5.15.0-37-generic
architecture:       64 bit
endianess:          little endian
----------------------------------- GPU -----------------------------------
vendor:             NVIDIA Corporation
model:              GeForce RTX 3070 Ti
driverVersion:      <unknown>
memory [MiB]:       -1
----------------------------------- RAM -----------------------------------
vendor:             <unknown>
model:              <unknown>
name:               <unknown>
serial-number:      <unknown>
size [MiB]:         64213.2
------------------------------- Main Board --------------------------------
vendor:             ASUSTeK COMPUTER INC.
name:               PRIME Z490-A
version:            Rev 1.xx
serial-number:      <unknown>
------------------------------- Batteries ---------------------------------
No Batteries installed or detected
--------------------------------- Disks -----------------------------------
Disk 0:
  vendor:           <unknown>
  model:            WDS500G3X0C-00SJG0
  serial-number:    2105EZ440111
  size:             -1
---------------------------------------------------------------------------
```

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
