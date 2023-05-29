cmake_minimum_required(VERSION 3.21)
project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

option(HWINFO_BUILD_EXAMPLES "Enabled HWinfo examples" ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

add_subdirectory(src)

if (HWINFO_BUILD_EXAMPLES)
    add_subdirectory(examples)

    include(CTest)
    add_subdirectory(test)

    add_test(Example examples/Example)
endif()
