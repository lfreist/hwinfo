cmake_minimum_required(VERSION 3.16)
project(hwinfo VERSION 1.0.0 LANGUAGES CXX)
include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})
option(NO_OCL "Disable OCL" OFF)

# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

if (NOT DEFINED NO_OCL)
    set(CMAKE_CXX_STANDARD 20)
else ()
    set(CMAKE_CXX_STANDARD 11)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

if (NOT DEFINED NO_OCL)
    set(OPENCL_SDK_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    include_directories(external/miss-opencl/include)
    include_directories(external/miss-opencl/external/OpenCL/external/OpenCL-CLHPP/include)
    if (NOT TARGET miss-opencl_static)
        add_subdirectory(external/miss-opencl)
    endif ()

    add_compile_definitions(USE_OCL)
    add_compile_definitions(NOMINMAX)
endif ()

include_directories({PROJECT_SOURCE_DIR}/include)
add_subdirectory(src)
target_include_directories(HWinfo PUBLIC ${PROJECT_SOURCE_DIR}/include/)

install(DIRECTORY include/hwinfo DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

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
