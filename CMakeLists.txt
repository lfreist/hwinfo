cmake_minimum_required(VERSION 3.22)

project(hwinfo VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (WIN32)
    add_definitions(-DWIN32 -DNOMINMAX)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wno-unknown-pragmas -Wno-overflow)
endif()

if(NOT HWINFO_CMAKE_BINARY_DIR)
    set(HWINFO_CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR})
endif()

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${HWINFO_CMAKE_BINARY_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${HWINFO_CMAKE_BINARY_DIR})

include(GNUInstallDirs)

string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}" PROJECT_IS_TOP_LEVEL)

option(HWINFO_SHARED "Shared build" ON)
option(HWINFO_STATIC "Static build" OFF)


option(BUILD_EXAMPLES "Build example program" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTING  "Build test program" ${PROJECT_IS_TOP_LEVEL})

# Components
option(HWINFO_OS "Enable OS information" ON)
option(HWINFO_COMPUTER "Enable computer information" ON)
option(HWINFO_MAINBOARD "Enable mainboard information" ON)
option(HWINFO_CPU "Enable CPU information" ON)
option(HWINFO_DISK "Enable disk information" ON)
option(HWINFO_RAM "Enable RAM information" ON)
option(HWINFO_GPU "Enable GPU information" ON)
option(HWINFO_GPU_BACKENDS "Load vendor GPU libraries (NVML, CUDA, Level Zero, OpenCL) at runtime, if installed" ON)
option(HWINFO_BATTERY "Enable battery information" ON)
option(HWINFO_NETWORK "Enable network information" ON)

if(NOT HWINFO_STATIC AND HWINFO_SHARED)
  set(BUILD_SHARED_LIBS ON)
else()
  set(BUILD_SHARED_LIBS OFF)
endif()

add_subdirectory(data)
set(HWINFO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_subdirectory(src)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
)
configure_file(cmake/lfreist-hwinfoConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        @ONLY
)
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/lfreist-hwinfoConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lfreist-hwinfo
)
install(EXPORT lfreist-hwinfoTargets
        FILE lfreist-hwinfoTargets.cmake
        NAMESPACE lfreist-hwinfo::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lfreist-hwinfo
)

if(BUILD_EXAMPLES OR BUILD_TESTING)
    add_subdirectory(examples)
endif()

if(BUILD_TESTING)
    include(CTest)
    add_subdirectory(test)
    add_test(NAME system_info COMMAND system_info)
endif()
