cmake_minimum_required(VERSION 3.22)

project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

if (WIN32)
    add_definitions(-DWIN32)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(NOT HWINFO_CMAKE_BINARY_DIR)
    set(HWINFO_CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR})
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${HWINFO_CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${HWINFO_CMAKE_BINARY_DIR})

# if standard not provided, use the latest supported by the compiler
if("${CMAKE_CXX_STANDARD}" STREQUAL "")
  function(_set_language_standard output language)
    foreach(version IN LISTS ARGN)
      if(DEFINED "CMAKE_${language}${version}_STANDARD_COMPILE_OPTION"
        OR DEFINED "CMAKE_${language}${version}_EXTENSION_COMPILE_OPTION"
      )
        set("${output}" "${version}" PARENT_SCOPE)
        break()
      endif()
    endforeach()
  endfunction()
  _set_language_standard(CMAKE_CXX_STANDARD CXX 20 17 14 11)
endif()

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(HWINFO_SHARED "Shared build" ON)
option(HWINFO_STATIC "Static build" OFF)


option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})

# Components
option(HWINFO_OS "Enable OS information" ON)
option(HWINFO_MAINBOARD "Enable mainboard information" ON)
option(HWINFO_CPU "Enable CPU information" ON)
option(HWINFO_DISK "Enable disk information" ON)
option(HWINFO_RAM "Enable RAM information" ON)
option(HWINFO_GPU "Enable GPU information" ON)
option(HWINFO_GPU_OPENCL "Enable OpenCL for more GPU information" OFF)
option(HWINFO_BATTERY "Enable battery information" ON)
option(HWINFO_NETWORK "Enable network information" ON)

if(NOT HWINFO_STATIC AND HWINFO_SHARED)
  set(HWINFO_BUILD SHARED)
else(HWINFO_STATIC)
  set(HWINFO_BUILD STATIC)
endif()

set(HWINFO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_subdirectory(src)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion
)
configure_file(cmake/lfreist-hwinfoConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        @ONLY
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hwinfo
)
install(EXPORT lfreist-hwinfoTargets
        FILE lfreist-hwinfoTargets.cmake
        NAMESPACE lfreist-hwinfo::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/hwinfo
)

if(BUILD_EXAMPLES OR BUILD_TESTING)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(system_info system_info)
endif()
