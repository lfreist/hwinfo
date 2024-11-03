cmake_minimum_required(VERSION 3.5)

project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

if (WIN32)
    add_definitions(-DWIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(BUILD_EXAMPLES "Build example program" OFF)
option(BUILD_TESTING  "Build test program" OFF)

if(NOT HWINFO_STATIC AND HWINFO_SHARED)
  set(HWINFO_BUILD SHARED)
else(HWINFO_STATIC)
  set(HWINFO_BUILD STATIC)
endif()

set(HWINFO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_subdirectory(src)

if(BUILD_EXAMPLES OR BUILD_TESTING)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(system_info system_info)
endif()
