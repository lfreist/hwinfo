# hwinfo
cross-platform C++ API for hardware information (CPU, RAM, GPU, ...)

## Content

* [Build hwinfo](#build-hwinfo)
* [Example](#example)
* [Include hwinfo to cmake project](#include-hwinfo-in-your-cmake-project)


## Build `hwinfo`
> Requirements: git, cmake, gcc

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