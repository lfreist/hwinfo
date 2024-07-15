cmake_minimum_required(VERSION 3.16)

# Use the latest C++ standard available
if("${CMAKE_CXX_STANDARD}" STREQUAL "")
    if(DEFINED CMAKE_CXX23_STANDARD_COMPILE_OPTION OR DEFINED CMAKE_CXX23_EXTENSION_COMPILE_OPTION)
        set(CMAKE_CXX_STANDARD 23)
    elseif(DEFINED CMAKE_CXX20_STANDARD_COMPILE_OPTION OR DEFINED CMAKE_CXX20_EXTENSION_COMPILE_OPTION)
        set(CMAKE_CXX_STANDARD 20)
    elseif(DEFINED CMAKE_CXX17_STANDARD_COMPILE_OPTION OR DEFINED
                                                            CMAKE_CXX17_EXTENSION_COMPILE_OPTION
    )
        set(CMAKE_CXX_STANDARD 17)
    elseif(DEFINED CMAKE_CXX14_STANDARD_COMPILE_OPTION OR DEFINED
                                                            CMAKE_CXX14_EXTENSION_COMPILE_OPTION
    )
        set(CMAKE_CXX_STANDARD 14)
    else()
        set(CMAKE_CXX_STANDARD 11)
    endif()
endif()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(hwinfo VERSION 1.0.0 LANGUAGES CXX)
include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})
option(NO_OCL "Disable OCL" OFF)

# Components
option(HWINFO_OS "Enable OS detection" ON)
option(HWINFO_MAINBOARD "Enable mainboard detection" ON)
option(HWINFO_CPU "Enable CPU detection" ON)
option(HWINFO_DISK "Enable disk detection" ON)
option(HWINFO_RAM "Enable RAM detection" ON)
option(HWINFO_GPU "Enable GPU detection" ON)
option(HWINFO_BATTERY "Enable battery detection" ON)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (NOT DEFINED NO_OCL)
    if (NOT TARGET miss-opencl_static)
        add_subdirectory(external/miss-opencl)
    endif ()

    add_compile_definitions(USE_OCL)
    add_compile_definitions(NOMINMAX)
endif ()

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
    add_test(Example examples/Example)
endif()
