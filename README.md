[![Linux (clang)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml)
[![Linux (gcc)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml)

[![MacOS](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml)

[![Windows (Visual Studio)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml)

[![clang format](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml)

# hwinfo

hwinfo provides an easy-to-use and modern C++ API for retrieving hardware information of your systems components such as
CPU, RAM, GPU, Disks, Mainboard, ...

hwinfo builds using C++20. However, if your compiler does not support C++20, you can fall back to C++11 by setting the
`NO_OCL` CMake variable (add `-DNO_OCL=ON` to the CMake command).

> **Note**
>
> If you face any issues, find bugs or if your platform is not supported yet, do not hesitate
> to [create an issue](https://github.com/lfreist/hwinfo/issues).

## Content

* [Supported Components](#supported-components)
* [API](#API)
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
|                  | Free Memory Size   |  ✔️   |   ❌   |    ❌    |
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
| Battery          | Vendor             |  ✔️   |   ❌   |    ❌    |
|                  | Model              |  ✔️   |   ❌   |    ❌    |
|                  | Serial Number      |  ✔️   |   ❌   |    ❌    |
|                  | Technology         |  ✔️   |   ❌   |    ❌    |
|                  | Capacity           |  ✔️   |   ❌   |   ️❌    |
|                  | Charging           |  ✔️   |   ❌   |    ❌    |

## API

This section describes, how you can get information about the supported components of your computer.

### CPU

hwinfo supports reading CPU information on boards with multiple sockets and CPUs installed.
`getAllSockets()` returns a `std::vector<Socket>`. A `Socket` object represents a physical socket and holds information
about the installed CPU. You can access these information via `Socket::CPU()` which retuns a `CPU` instance.

> Why not just retrieving a `std::vector<CPU>`?
> The reason for this lies within how linux handles CPUs. For linux systems, the cores of a multi core CPU are
> considered as different physical CPUs.
> Thus, I added the `Socket` layer to make clear, that multiple elements in the yielded `std::vector<Socket>` vector
> mean that there are two CPUs on two different sockets installed.

The following methods are available for `CPU`:

- `const std::string& CPU::vendor() const` "GenuineIntel"
- `const std::string& CPU::modelName() const` "Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz"
- `int64_t CPU::cacheSize_Bytes() const` 16384000
- `int CPU::numPhysicalCores() const` 8
- `int CPU::numLogicalCores() const` 16
- `int64_t CPU::maxClockSpeed_MHz() const` 5100000
- `int64_t CPU::regularClockSpeed_MHz() const` 3800000
- `int64_t CPU::minClockSpeed_MHz() const` 1800000
- `int64_t CPU::currentClockSpeed_MHz() const` 4700189
- `const std::vector<std::string>& CPU::flags() cosnt` {"SSE", "AVX", ...}

### GPU

You can also get information about all installed GPUs using hwinfo.
`getAllGPUs()` returns a `std::vector<GPU>`. A `GPU` object represents a physical GPU.

The following methods are available for `GPU`:

- `const std::string& GPU::vendor() const` "NVIDIA"
- `const std::string& GPU::name() const` "NVIDIA GeForce RTX 3070 Ti"
- `const std::string& GPU::driverVersion() const` "31.0.15.2698" (empty for linux)
- `int64_t GPU::memory_Bytes() const` 8190000000
- `int64_t GPU::min_frequency_MHz() const`
- `int64_t GPU::current_frequency_MHz() const`
- `int64_t GPU::max_frequency_MHz() const`
- `int GPU::id() const` 0

### RAM

TODO

### OS

TODO

### Baseboard

TODO

### Disk

TODO

## Build `hwinfo`

> Requirements: git, cmake, c++ compiler (gcc, clang, MSVC)

1. Download repository:
    ```
    git clone https://github.com/lfreist/hwinfo
    ```
2. Build using cmake:
    ```bash
    mkdir build
    cmake -B build -DCMAKE_BUILD_TYPE=Release  # -DNO_OCL=ON (for C++11)
    cmake --build build
    ```

## Example

See [example.cpp](examples/example.cpp)

The output should look similar to this one:

```
Hardware Report:

----------------------------------- CPU -----------------------------------
Socket 0:
 vendor:            GenuineIntel
 model:             Intel(R) Core(TM) i7-10700K CPU @ 3.80GHz
 physical cores:    8
 logical cores:     16
 max frequency:     3792
 regular frequency: 3792
 min frequency:     -1
 current frequency: 3792
 cache size:        16777216
----------------------------------- OS ------------------------------------
Operating System:   Microsoft Windows 11 Professional (build 22621)
short name:         Windows
version:            <unknown>
kernel:             <unknown>
architecture:       64 bit
endianess:          little endian
----------------------------------- GPU -----------------------------------
GPU 0:
  vendor:           NVIDIA
  model:            NVIDIA GeForce RTX 3070 Ti
  driverVersion:    31.0.15.2698
  memory [MiB]:     8190
  min frequency:    0
  cur frequency:    0
  max frequency:    0
----------------------------------- RAM -----------------------------------
vendor:             Corsair
model:              CMK32GX4M2Z3600C18
name:               Physical Memory
serial-number:      ***
size [MiB]:         65437
free [MiB]:         54405
available [MiB]:    54405
------------------------------- Main Board --------------------------------
vendor:             ASUSTeK COMPUTER INC.
name:               PRIME Z490-A
version:            Rev 1.xx
serial-number:      ***
------------------------------- Batteries ---------------------------------
No Batteries installed or detected
--------------------------------- Disks -----------------------------------
Disk 0:
  vendor:           (Standard disk drives)
  model:            WD_BLACK SN850 Heatsink 1TB
  serial-number:    ***.
  size:             1000202273280
Disk 1:
  vendor:           (Standard disk drives)
  model:            Intenso SSD Sata III
  serial-number:    ***
  size:             120031511040
Disk 2:
  vendor:           (Standard disk drives)
  model:            KINGSTON SA400S37240G
  serial-number:    ***
  size:             240054796800
Disk 3:
  vendor:           (Standard disk drives)
  model:            WDS500G3X0C-00SJG0
  serial-number:    ***.
  size:             500105249280
Disk 4:
  vendor:           (Standard disk drives)
  model:            ST750LM022 HN-M750MBB
  serial-number:    ***
  size:             750153761280
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
