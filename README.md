[![Linux (clang)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml)
[![Linux (gcc)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml)

[![MacOS](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml)

[![Windows (MinGW)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml)
[![Windows (Visual Studio)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml)

[![clang format](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml)

# hwinfo

hwinfo provides an easy-to-use and modern C++ API for retrieving hardware information of your systems components such as
CPU, RAM, GPU, Disks, Mainboard, ...

> **Note**
>
> If you face any issues, find bugs or if your platform is not supported yet, do not hesitate
> to [create an issue](https://github.com/lfreist/hwinfo/issues).

## Content

* [Supported Components and API](#supported-components-and-api)
* [Build hwinfo](#build-hwinfo)
* [Example](#example)
* [Include hwinfo to cmake project](#include-hwinfo-in-your-cmake-project)

## Supported Components and API

> **Note**
>
> The listed components that are not yet implemented (indicated with ❌) are in development and will be supported in
> later releases. **You are welcome to start contributing and help improving this library!**

| Component        | Info               | Linux  | Apple | Windows | API                                                                   |
|------------------|:-------------------|:------:|:-----:|:-------:|:----------------------------------------------------------------------|
| CPU              | Vendor             |   ✔️   |  ✔️   |   ✔️    | `CPU::getVendor()`                                                    |
|                  | Model              |   ✔️   |  ✔️   |   ✔️    | `CPU::getModelName()`                                                 |
|                  | Frequency          |   ✔️   |  ✔️   |   ✔️    | `CPU::getRegularClockSpeed_kHz()` </br> `CPU::getMaxClockSpeed_kHz()` |
|                  | Physical Cores     |   ✔️   |  ✔️   |   ✔️    | `CPU::getNumPhysicalCores()`                                          |
|                  | Logical Cores      |   ✔️   |  ✔️   |   ✔️    | `CPU::getNumLogicalCores()`                                           |
|                  | Cache Size         |   ✔️   |  ✔️   |   ✔️    | `CPU::getCacheSize_Bytes()`                                           |
| GPU              | Vendor             |   ✔️   |  ✔️   |   ✔️    | `GPU::getVendor()`                                                    |
|                  | Model              |   ✔️   |  ✔️   |   ✔️    | `GPU::getName()`                                                      |
|                  | Memory Size        |   ❌    |   ❌   |   ✔️    | `GPU::getMemory_Bytes()`                                              |
| Memory (RAM)     | Vendor             |   ❌    |   ❌   |   ✔️    | `RAM::getVendor()`                                                    |
|                  | Model              |   ❌    |   ❌   |   ✔️    | `RAM::getModel()`                                                     |
|                  | Name               |   ❌    |   ❌   |   ✔️    | `RAM::getName()`                                                      |
|                  | Serial Number      |   ❌    |   ❌   |   ✔️    | `RAM::get()`                                                          |
|                  | Total Memory Size  |   ✔️   |  ✔️   |   ✔️    | `RAM::getTotalSizes_Bytes()`                                          |
|                  | Free Memory Size   |   ✔️   |   ❌   |    ❌    | `-`                                                                   |
| Mainboard        | Vendor             |   ✔️   |   ❌   |   ✔️    | `MainBoard::getVendor()`                                              |
|                  | Model              |   ✔️   |   ❌   |   ✔️    | `MainBoard::getName()`                                                |
|                  | Version            |   ✔️   |   ❌   |   ✔️    | `MainBoard::getVersion()`                                             |
|                  | Serial-Number      |   ❌    |   ❌   |   ✔️    | `MainBoard::getSerialNumber()`                                        |
|                  | Bios               |   ❌    |   ❌   |    ❌    | `-`                                                                   |
| Disk             | Vendor             |   ✔️   |   ❌   |   ✔️    | `Disk::vendor()`*                                                     |
|                  | Model              |   ✔️   |   ❌   |   ✔️    | `Disk::model()`*                                                      |
|                  | Serial-Number      |   ❌    |   ❌   |   ✔️    | `Disk::serialNumber`*                                                 |
|                  | Size               |   ❌    |   ❌   |    ❌    | `Disk::size_Bytes()`*                                                 |
| Operating System | Name               |   ✔️   |  ✔️   |   ✔️    | `OS::getFullName()`                                                   |
|                  | Short Name         |   ✔️   |  ✔️   |   ✔️    | `OS::getName()`                                                       |
|                  | Version            |   ✔️   |  ✔️   |    ❌    | `OS::getVersion()`                                                    |
|                  | Kernel             |   ✔️   |   ❌   |    ❌    | `OS::getKernel()`                                                     |
|                  | Architecture (Bit) |   ✔️   |  ✔️   |   ✔️    | `OS::getIs32bit()` </br> `OS::getIs64bit()`                           |
|                  | Endianess          |   ✔️   |  ✔️   |   ✔️    | `OS::getIsBigEndian()` </br> `OS::getIsLittleEndian()`                |
| Battery          | Vendor             |   ✔️   |   ❌   |    ❌    | `Battery::vendor()`*                                                  |
|                  | Model              |   ✔️   |   ❌   |    ❌    | `Battery::model()`*                                                   |
|                  | Serial Number      |   ✔️   |   ❌   |    ❌    | `Battery::serialNumber()`*                                            |
|                  | Technology         |   ✔️   |   ❌   |    ❌    | `Battery::technology()`*                                              |
|                  | Capacity           |   ✔️   |   ❌   |   ️❌    | `Battery::capacity()`*                                                |
|                  | Charging           |   ✔️   |   ❌   |    ❌    | `Battery::charging()` </br> `Battery::discharging()`*                 |

> *Disks must be initialized using `getAllDisks()`
>
> *Batteries should be initialized using `getAllBatteries()` or an ID must be provided: `Battery::Battery(int8_t id)`

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
    // file: your_executable.cpp
    
    #include "hwinfo/hwinfo.h"

   int main(int argc, char** argv) {
     // Your code
   }
    ```
4. Link it in cmake
    ```cmake
    add_executable(your_executable your_executable.cpp)
    target_link_libraries(your_executable PUBLIC hwinfo::HWinfo)
    ```
