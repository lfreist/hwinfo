> **Warning**
> hwinfo.io is not affiliated with this repository.
> Any claim that this is the official product repository of hwinfo.io is false.

[![Linux (clang)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-clang.yml)
[![Linux (gcc)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-linux-gcc.yml)

[![MacOS](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-macos.yml)

[![Windows (Visual Studio)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-vs.yml)
[![Windows (MinGW)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/build-windows-mingw.yml)

[![clang format](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml/badge.svg)](https://github.com/lfreist/hwinfo/actions/workflows/format-check.yml)

# hwinfo

hwinfo provides an easy-to-use and modern C++23 API for retrieving hardware information of your system's components such
as CPU, RAM, GPU, disks, mainboard, battery and network interfaces, and for monitoring their utilization.

```c++
#include <hwinfo/hwinfo.h>

#include <print>

int main() {
  auto cpus = hwinfo::cpus();  // hwinfo::result<std::vector<hwinfo::Cpu>>, i.e. std::expected<..., hwinfo::error>
  if (!cpus) {
    std::println(stderr, "{}", cpus.error());  // e.g. "cannot read /proc/cpuinfo: No such file or directory"
    return 1;
  }
  for (const auto& cpu : *cpus) {
    std::println("{} {}: {} cores, L3 {}", cpu.vendor, cpu.model, cpu.physical_cores,
                 cpu.cores.front().cache.l3.value_or(hwinfo::Bytes{}));
  }
  if (auto os = hwinfo::os()) {
    std::println("{}", *os);  // every type is formattable: "Ubuntu 24.04.1 LTS (x86_64, kernel 6.8.0-45-generic)"
  }
}
```

> **Note**
>
> If you face any issues, find bugs or if your platform is not supported yet, do not hesitate
> to [create an issue](https://github.com/lfreist/hwinfo/issues).

## Content

- [hwinfo](#hwinfo)
  - [Content](#content)
  - [Requirements](#requirements)
  - [Usage](#usage)
    - [Queries](#queries)
    - [Error handling](#error-handling)
    - [Units](#units)
    - [Monitoring](#monitoring)
  - [Supported Components](#supported-components)
  - [Build `hwinfo`](#build-hwinfo)
  - [Example](#example)
  - [include hwinfo to cmake project](#include-hwinfo-to-cmake-project)
    - [Include installed version](#include-installed-version)
    - [As git submodule](#as-git-submodule)
  - [Migrating from hwinfo 1.x](#migrating-from-hwinfo-1x)

## Requirements

hwinfo 2 requires C++23 (`std::expected`, `std::format`, ranges). Tested compilers:

- GCC 14 or newer
- Clang 20 or newer (libstdc++ 14 or libc++ 20)
- Apple Clang 16 (Xcode 16) or newer
- MSVC 19.40 (Visual Studio 2022 17.10) or newer
- MinGW-w64 with GCC 14 or newer

## Usage

### Queries

Every component is a plain aggregate returned by a free function:

| Function                       | Returns                                        | Header              |
|--------------------------------|------------------------------------------------|---------------------|
| `hwinfo::computer()`           | `result<Computer>` (vendor, model, chassis)    | `hwinfo/computer.h` |
| `hwinfo::cpus()`               | `result<std::vector<Cpu>>` (one per socket)    | `hwinfo/cpu.h`      |
| `hwinfo::memory()`             | `result<Memory>` (total + installed modules)   | `hwinfo/ram.h`      |
| `hwinfo::gpus()`               | `result<std::vector<Gpu>>`                     | `hwinfo/gpu.h`      |
| `hwinfo::disks()`              | `result<std::vector<Disk>>`                    | `hwinfo/disk.h`     |
| `hwinfo::os()`                 | `result<Os>`                                   | `hwinfo/os.h`       |
| `hwinfo::mainboard()`          | `result<Mainboard>`                            | `hwinfo/mainboard.h`|
| `hwinfo::batteries()`          | `result<std::vector<Battery>>`                 | `hwinfo/battery.h`  |
| `hwinfo::network_interfaces()` | `result<std::vector<NetworkInterface>>`        | `hwinfo/network.h`  |

Values are regular types (copyable, comparable, formattable), which makes them easy to use with ranges:

```c++
using namespace hwinfo::literals;

auto large_disks = hwinfo::disks().transform([](std::vector<hwinfo::Disk> disks) {
  return disks | std::views::filter([](const hwinfo::Disk& d) { return d.size > 1_TiB; })
               | std::ranges::to<std::vector>();
});

bool avx2 = hwinfo::cpus().transform([](const auto& cpus) { return cpus.front().has_flag("avx2"); }).value_or(false);
```

Information that the platform does not expose, or that requires elevated privileges, is a `std::optional` (e.g.
`Disk::serial_number`, `Gpu::driver_version`, `Mainboard::serial_number`) instead of a placeholder string.

`hwinfo::computer()` identifies the machine as a product (e.g. a notebook or prebuilt PC) and gives access to its
components. Each member function queries on call, so only the components you use need to be linked:

```c++
if (auto pc = hwinfo::computer()) {
  std::println("{} ({})", *pc, pc->chassis);  // "LENOVO 21CBCTO1WW (laptop)"
  auto gpus = pc->gpus();                      // same as hwinfo::gpus()
}
```

### Error handling

All queries return `hwinfo::result<T>`, an alias for `std::expected<T, hwinfo::error>`. A query fails only if its
data source is unavailable (e.g. `/proc/cpuinfo` cannot be read or the WMI connection fails); attributes that cannot be
determined individually are `std::nullopt`. An empty vector means that no such device is present.

`hwinfo::error` wraps a `std::error_code` (keeping the original `errno` / Win32 / IOKit code) plus a context string, and
compares equal to the portable conditions in `hwinfo::errc`:

```c++
auto board = hwinfo::mainboard();
if (!board && board.error() == hwinfo::errc::not_supported) {
  // e.g. no DMI information on this ARM board
} else if (!board && board.error() == hwinfo::errc::permission_denied) {
  // ...
}
std::println("{}", board.error());                 // "<context>: <message>"
std::error_code ec = board.error().code();         // the underlying code
```

### Units

Sizes, frequencies, data rates and energies are strong types: `hwinfo::Bytes`, `hwinfo::Hertz`, `hwinfo::DataRate`
and `hwinfo::Energy`. They are formatted with automatic
scaling, support an explicit unit and precision in the format spec, and can be converted to any unit:

```c++
using namespace hwinfo::literals;

std::println("{}", memory->total);                   // "31.1 GiB"
std::println("{:.0MiB}", memory->total);             // "31854 MiB"
std::println("{:>12.3GHz}", *core.max_frequency);    // "   5.400 GHz"
double gib = memory->total.to(hwinfo::ByteUnit::GiB);
bool large = memory->total > 16_GiB;
```

### Monitoring

Dynamic values live in `hwinfo/monitoring.h`:

```c++
hwinfo::CpuSampler sampler;                          // takes a baseline, no global state
std::this_thread::sleep_for(500ms);
auto load = sampler.sample();                        // result<CpuLoad>: total and per-thread utilization in [0, 1]

auto frequencies = hwinfo::cpu_frequencies();        // result<std::vector<Hertz>>
auto ram = hwinfo::memory_usage();                   // result<MemoryUsage>: total, free, available
auto space = hwinfo::disk_space("/");                // result<DiskSpace>: capacity, free, available
auto battery = hwinfo::battery_status(0);            // result<BatteryStatus>: state, charge in [0, 1]

// periodic updates on a background thread (std::jthread), stopped on destruction
hwinfo::Monitor monitor{1s, [s = hwinfo::CpuSampler{}]() mutable { return s.sample(); },
                        [](const hwinfo::result<hwinfo::CpuLoad>& load) {
                          if (load) std::println("{:.1f}%", load->total * 100);
                        }};
```

See [live_monitorMain.cpp](examples/live_monitorMain.cpp) for a complete example.

## Supported Components

> **Note**
>
> The listed components that are not yet implemented (indicated with ❌) are in development and will be supported in
> future releases. **You are welcome to start contributing and help improving this library!**

| Component        | Info | Linux | Apple | Windows |
|---|---|:---:|:---:|:---:|
| CPU | Vendor, model | ✔️ | ✔️ | ✔️ |
|  | Physical / logical cores | ✔️ | ✔️ | ✔️ |
|  | Cache sizes | ✔️ | ✔️ | ✔️ |
|  | Base / max frequency | ✔️ | Intel only | ✔️ |
|  | Feature flags | ✔️ | ✔️ | ✔️ |
|  | Utilization (`CpuSampler`) | ✔️ | ✔️ | ✔️ |
|  | Current frequency | ✔️ | Intel (nominal) | ✔️ |
| GPU | Vendor, model, PCI id | ✔️ | ✔️ | ✔️ |
|  | Driver version | ✔️ | ❌ | ✔️ |
|  | Memory size | amdgpu | ✔️ | ✔️ |
| Memory (RAM) | Total | ✔️ | ✔️ | ✔️ |
|  | Modules (vendor, model, serial, size, frequency) | ❌ | ❌ | ✔️ |
|  | Free / available | ✔️ | ✔️ | ✔️ |
| Computer | Vendor, model, serial number | ✔️ | ✔️ | ✔️ |
|  | Family, version, SKU | ✔️ | family (marketing name) | ✔️ |
|  | Chassis type | ✔️ | from model | ✔️ |
| Mainboard | Vendor, name | ✔️ | ✔️ | ✔️ |
|  | Version | ✔️ | ❌ | ✔️ |
|  | Serial number | root | ✔️ | ✔️ |
| Disk | Vendor, model, serial number | ✔️ | ✔️ | ✔️ |
|  | Size, bus type | ✔️ | ✔️ | ✔️ |
|  | Mount points | ✔️ | ✔️ | ✔️ |
|  | Free space (`disk_space`) | ✔️ | ✔️ | ✔️ |
| Operating System | Name, version, kernel | ✔️ | ✔️ | ✔️ |
|  | Architecture, bits | ✔️ | ✔️ | ✔️ |
| Battery | Vendor, model, serial number | ✔️ | ✔️ | ✔️ |
|  | Technology | ✔️ | ❌ | ✔️ |
|  | Capacity | ✔️ | ✔️ | ✔️ |
|  | State, charge | ✔️ | ✔️ | ✔️ |
| Network | Name, index, MAC, IPv4 / IPv6, state | ✔️ | ✔️ | ✔️ |
|  | Description | ❌ | ❌ | ✔️ |

All components are available via the `lfreist-hwinfo::hwinfo` target, or via individual CMake targets, which you can
choose and link against depending on your needs.

```cmake
target_link_libraries(your_target PRIVATE lfreist-hwinfo::hwinfo)
```

or

```cmake
target_link_libraries(
  your_target
  PRIVATE lfreist-hwinfo::computer
          lfreist-hwinfo::cpu
          lfreist-hwinfo::gpu
          lfreist-hwinfo::ram
          lfreist-hwinfo::mainboard
          lfreist-hwinfo::disk
          lfreist-hwinfo::os
          lfreist-hwinfo::battery
          lfreist-hwinfo::network)
```

The CMake options control which components will be built and available in the library:

- `HWINFO_OS` "Enable OS detection" (default to `ON`)
- `HWINFO_COMPUTER` "Enable computer detection" (default to `ON`)
- `HWINFO_MAINBOARD` "Enable mainboard detection" (default to `ON`)
- `HWINFO_CPU` "Enable CPU detection" (default to `ON`)
- `HWINFO_DISK` "Enable disk detection" (default to `ON`)
- `HWINFO_RAM` "Enable RAM detection" (default to `ON`)
- `HWINFO_GPU` "Enable GPU detection" (default to `ON`)
- `HWINFO_GPU_OPENCL` "Enable usage of OpenCL in GPU information" (default to `OFF`)
- `HWINFO_BATTERY` "Enable battery detection" (default to `ON`)
- `HWINFO_NETWORK` "Enable network interface detection" (default to `ON`)

The monitoring functions are part of their component's library: `CpuSampler` and `cpu_frequencies()` of `cpu`,
`memory_usage()` of `ram` and `battery_status()` of `battery`; `disk_space()` and `Monitor` are header-only.

## Build `hwinfo`

> Requirements: git, cmake (>= 3.22), a C++23 compiler (see [Requirements](#requirements))

1. Download repository:
    ```
    git clone https://github.com/lfreist/hwinfo
    ```
2. Build using cmake:
    ```bash
    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
    ```
   Shared libraries are built by default; pass `-DHWINFO_STATIC=ON` for static libraries.
3. Run the tests (GoogleTest is downloaded automatically if it is not installed):
    ```bash
    ctest --test-dir build -C Release --output-on-failure
    ```

## Example

See [system_infoMain.cpp](examples/system_infoMain.cpp)

The output should look similar to this one:

```
------------------------------------- CPU --------------------------------------
Socket 0
  vendor:               GenuineIntel
  model:                13th Gen Intel(R) Core(TM) i7-13700H
  cores:                14 (20 threads)
  flags:                138 flags
  L1d / L1i:            48.0 KiB / 32.0 KiB
  L2 / L3:              1.2 MiB / 24.0 MiB
  base frequency:       2.40 GHz
  max frequency:        4.80 GHz
------------------------------- Operating System -------------------------------
  name:                 Ubuntu
  version:              26.04.1 LTS (Resolute Raccoon)
  kernel:               7.0.0-34-generic
  architecture:         x86_64 (64-bit)
------------------------------------- GPU --------------------------------------
GPU 0
  vendor:               NVIDIA Corporation
  model:                AD106M [GeForce RTX 4070 Max-Q / Mobile]
  driver:               nvidia
  driver version:       595.91.07
  memory:               <unknown>
  frequency:            <unknown>
  cores:                <unknown>
GPU 1
  vendor:               Intel Corporation
  model:                Raptor Lake-P [Iris Xe Graphics]
  driver:               i915
  driver version:       <unknown>
  memory:               <unknown>
  frequency:            1.50 GHz
  cores:                <unknown>
------------------------------------ Memory ------------------------------------
  total:                14.9 GiB
---------------------------------- Mainboard -----------------------------------
  vendor:               NB02
  name:                 PH6PG01_PH6PG71
  version:              Standard
  serial number:        ***
---------------------------------- Batteries -----------------------------------
Battery 0
  vendor:               OEM
  model:                standard
  serial number:        ***
  technology:           Li-ion
  capacity [Wh]:        82.044
  state:                discharging
  charge:               65%
------------------------------------ Disks -------------------------------------
Disk 0
  vendor:               <unknown>
  model:                SAMSUNG MZVL2512HDJD-00B07
  serial number:        ***
  bus:                  NVMe
  size:                 476.9 GiB
  mount points:         /, /boot/efi
Disk 1
  vendor:               <unknown>
  model:                KINGSTON SFYRS1000G
  serial number:        ***
  bus:                  NVMe
  size:                 931.5 GiB
  mount points:         
----------------------------------- Network ------------------------------------
Interface 2
  name:                 wlo1
  description:          <unknown>
  state:                up
  mac:                  ***
  ipv4:                 ***
  ipv6:                 ***
```

## include hwinfo to cmake project

### Include installed version

1. Install hwinfo
   ```
   git clone https://github.com/lfreist/hwinfo && cd hwinfo
   cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
   cmake --install build
   ```
2. Simply add the following to your `CMakeLists.txt` file:
    ```cmake
    # file: CMakeLists.txt

    find_package(lfreist-hwinfo 2 REQUIRED)
    ```
3. Include `hwinfo` into your `.cpp/.h` files:
    ```c++
    // file: your_executable.cpp

    #include <hwinfo/hwinfo.h>

    int main(int argc, char** argv) {
      // Your code
    }
    ```
4. Link it in cmake
    ```cmake
    add_executable(your_executable your_executable.cpp)
    target_link_libraries(your_executable PUBLIC lfreist-hwinfo::hwinfo)
    ```

### As git submodule

1. Download `hwinfo` into your project (e.g. in `<project-root>/third_party/hwinfo`)
    ```
    mkdir third_party
    cd third_party
    git clone https://github.com/lfreist/hwinfo
    ```
2. Simply add the following to your `<project-root>/CMakeLists.txt` file:
    ```cmake
    # file: <project-root>/CMakeLists.txt

    # define the HWINFO_* options if you want to change the default values

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
    target_link_libraries(your_executable PUBLIC lfreist-hwinfo::hwinfo)
    ```

## Migrating from hwinfo 1.x

hwinfo 2 is a breaking release. The most important changes:

| hwinfo 1.x                                                  | hwinfo 2                                                           |
|-------------------------------------------------------------|--------------------------------------------------------------------|
| C++17                                                       | C++23                                                              |
| `getAllCPUs()`, `getAllDisks()`, `getAllGPUs()`, ...        | `cpus()`, `disks()`, `gpus()`, `batteries()`, `network_interfaces()` |
| `OS os;`, `MainBoard board;`, `Memory memory;` (constructors) | `os()`, `mainboard()`, `memory()`                                  |
| classes with getters (`cpu.modelName()`)                    | aggregates with public members (`cpu.model`)                       |
| `"<unknown>"`, `-1`, `0` on failure                         | `std::expected` for failed queries, `std::optional` for missing values |
| `uint64_t` bytes / Hz, `unit_prefix_to(...)`                | `hwinfo::Bytes` / `hwinfo::Hertz`, formattable, `.to(ByteUnit::GiB)` |
| `operator<<`                                                | `std::formatter` (`std::format`, `std::print`)                     |
| `hwinfo::monitor::cpu::*`, `hwinfo::monitoring::{cpu,ram,disk}::*` | `CpuSampler`, `cpu_frequencies()`, `memory_usage()`, `disk_space()`, `battery_status()` in `hwinfo/monitoring.h` |
| `Memory::free()` / `available()`, `Battery::capacity()` / `state()` | `memory_usage()`, `battery_status(index)`                     |
| `OS::isBigEndian()` / `isLittleEndian()`                    | `std::endian::native`                                              |
| `Disk::Interface::USB3_10GBit`, ...                         | `DiskBus::usb` + `Disk::link_speed_gbps`                           |
| `hwinfo/utils/*.h` (string utilities, WMI, sysctl, ...)     | removed from the public API                                        |
| `find_package(hwinfo)`, `lfreist-hwinfo::hwinfo_cpu`        | `find_package(lfreist-hwinfo)`, `lfreist-hwinfo::cpu`              |
