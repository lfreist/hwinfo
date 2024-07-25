cmake_minimum_required(VERSION 3.16)

project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

if (WIN32)
    add_definitions(-DWIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})

# Components
option(HWINFO_OS "Enable OS detection" ON)
option(HWINFO_MAINBOARD "Enable mainboard detection" ON)
option(HWINFO_CPU "Enable CPU detection" ON)
option(HWINFO_DISK "Enable disk detection" ON)
option(HWINFO_RAM "Enable RAM detection" ON)
option(HWINFO_GPU "Enable GPU detection" ON)
option(HWINFO_GPU_OPENCL "Enable usage of OpenCL in GPU information" OFF)
option(HWINFO_BATTERY "Enable battery detection" ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(HWINFO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_subdirectory(src)

if (NOT PROJECT_IS_TOP_LEVEL)
    return()
endif()

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(system_infoMain system_infoMain)
endif()
