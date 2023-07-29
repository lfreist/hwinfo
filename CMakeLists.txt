cmake_minimum_required(VERSION 3.16)
project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")

set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

if (NOT DEFINED NO_OCL)
    set(OPENCL_SDK_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(external/OpenCL)
    include_directories(external/OpenCL/external/OpenCL-CLHPP/include)

    add_compile_definitions(USE_OCL)
endif ()

include_directories(${PROJECT_SOURCE_DIR}/include/)

add_subdirectory(src)

if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    file(COPY "scripts/pci.ids" DESTINATION "$ENV{HOME}/.hwinfo/")
endif ()

if (${MAIN_PROJECT})
    add_subdirectory(examples)

    include(CTest)
    add_subdirectory(test)

    add_test(Example examples/Example)
endif()
