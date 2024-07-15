# hwinfo (all in one target)
add_library(hwinfo hwinfo.cpp)
add_library(hwinfo::hwinfo ALIAS hwinfo)

# for backward compatibility
add_library(HWinfo ALIAS hwinfo)
add_library(hwinfo::HWinfo ALIAS hwinfo)

target_include_directories(hwinfo PUBLIC ${PROJECT_SOURCE_DIR}/include/)

# C++ options based on CMake options
set(HWINFO_DEFINITIONS "")
set(OPTIONS_LIST
    HWINFO_OS
    HWINFO_MAINBOARD
    HWINFO_CPU
    HWINFO_DISK
    HWINFO_RAM
    HWINFO_GPU
    HWINFO_BATTERY)
foreach(OPTION IN LISTS OPTIONS_LIST)
  if(${OPTION})
    list(APPEND HWINFO_DEFINITIONS ${OPTION})
  endif()
endforeach()

target_compile_definitions(hwinfo PUBLIC ${HWINFO_DEFINITIONS})

install(TARGETS hwinfo DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/hwinfo DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# filesystem_utils
add_library(
  hwinfo_utils_filesystem
  # apple specific definitions
  apple/utils/filesystem.cpp
  # linux specific definitions
  linux/utils/filesystem.cpp
  # windows specific definitions
  windows/utils/filesystem.cpp)
add_library(hwinfo::utils::filesystem ALIAS hwinfo_utils_filesystem)

target_include_directories(
  hwinfo_utils_filesystem
  PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(hwinfo_utils_filesystem PUBLIC ${HWINFO_DEFINITIONS})

# wmi_wrapper on windows
add_library(
  hwinfo_utils_wmi
  # windows specific definitions
  windows/utils/wmi_wrapper.cpp)
add_library(hwinfo::utils::wmi ALIAS hwinfo_utils_wmi)

if(WIN32 AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
  target_link_libraries(hwinfo_utils_wmi PRIVATE "wbemuuid")
endif()

target_include_directories(
  hwinfo_utils_wmi PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(hwinfo_utils_wmi PUBLIC ${HWINFO_DEFINITIONS})

# PCIMapper on Unix
add_library(hwinfo_utils_PCIMapper PCIMapper.cpp)
add_library(hwinfo::utils::PCIMapper ALIAS hwinfo_utils_PCIMapper)

target_include_directories(
  hwinfo_utils_PCIMapper
  PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_compile_definitions(hwinfo_utils_PCIMapper PUBLIC ${HWINFO_DEFINITIONS})

# battery
if(HWINFO_BATTERY)
  add_library(
    hwinfo_battery
    battery.cpp
    # apple specific definitions
    apple/battery.cpp
    # linux specific definitions
    linux/battery.cpp
    # windows specific definitions
    windows/battery.cpp)
  add_library(hwinfo::battery ALIAS hwinfo_battery)

  target_include_directories(
    hwinfo_battery PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_libraries(hwinfo_battery PRIVATE hwinfo::utils::filesystem
                                               hwinfo::utils::wmi)

  target_compile_definitions(hwinfo_battery PUBLIC ${HWINFO_DEFINITIONS})

  install(TARGETS hwinfo_battery DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_link_libraries(hwinfo PUBLIC hwinfo_battery)
endif()

# cpu
if(HWINFO_CPU)
  add_library(
    hwinfo_cpu
    cpu.cpp
    # apple specific definitions
    apple/cpu.cpp
    # linux specific definitions
    linux/cpu.cpp
    # windows specific definitions
    windows/cpu.cpp)
  add_library(hwinfo::cpu ALIAS hwinfo_cpu)

  target_include_directories(
    hwinfo_cpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_libraries(hwinfo_cpu PRIVATE hwinfo::utils::filesystem
                                           hwinfo::utils::wmi)

  target_compile_definitions(hwinfo_cpu PUBLIC ${HWINFO_DEFINITIONS})

  target_link_libraries(hwinfo PUBLIC hwinfo_cpu)

  install(TARGETS hwinfo_cpu DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# disk
if(HWINFO_DISK)
  add_library(
    hwinfo_disk
    disk.cpp
    # apple specific definitions
    apple/disk.cpp
    # linux specific definitions
    linux/disk.cpp
    # windows specific definitions
    windows/disk.cpp)
  add_library(hwinfo::disk ALIAS hwinfo_disk)

  target_include_directories(
    hwinfo_disk PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                       $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_compile_definitions(hwinfo_disk PUBLIC ${HWINFO_DEFINITIONS})

  target_link_libraries(hwinfo_disk PRIVATE hwinfo::utils::filesystem
                                            hwinfo::utils::wmi)

  if(APPLE)
    target_link_libraries(hwinfo_disk PRIVATE "-framework IOKit"
                                              "-framework CoreFoundation")
  endif()

  install(TARGETS hwinfo_disk DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_link_libraries(hwinfo PUBLIC hwinfo_disk)
endif()

# gpu
if(HWINFO_GPU)
  add_library(
    hwinfo_gpu
    gpu.cpp
    # apple specific definitions
    apple/gpu.cpp
    # linux specific definitions
    linux/gpu.cpp
    # windows specific definitions
    windows/gpu.cpp)
  add_library(hwinfo::gpu ALIAS hwinfo_gpu)

  target_include_directories(
    hwinfo_gpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_libraries(
    hwinfo_gpu PRIVATE hwinfo::utils::filesystem hwinfo::utils::wmi
                       hwinfo::utils::PCIMapper)

  target_compile_definitions(hwinfo_gpu PUBLIC ${HWINFO_DEFINITIONS})

  if(HWINFO_GPU_OPENCL)
    if(NOT TARGET miss-opencl_static)
      add_subdirectory(${PROJECT_SOURCE_DIR}/external/miss-opencl)
    endif()

    target_compile_definitions(hwinfo_gpu PRIVATE USE_OCL NOMINMAX)

    target_link_libraries(hwinfo_gpu PRIVATE miss-opencl_static)
  endif()

  install(TARGETS hwinfo_gpu DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_link_libraries(hwinfo PUBLIC hwinfo_gpu)
endif()

# mainboard
if(HWINFO_MAINBOARD)
  add_library(
    hwinfo_mainboard
    mainboard.cpp
    # apple specific definitions
    apple/mainboard.cpp
    # linux specific definitions
    linux/mainboard.cpp
    # windows specific definitions
    windows/mainboard.cpp)
  add_library(hwinfo::mainboard ALIAS hwinfo_mainboard)

  target_include_directories(
    hwinfo_mainboard PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                            $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_compile_definitions(hwinfo_mainboard PUBLIC ${HWINFO_DEFINITIONS})

  target_link_libraries(hwinfo_mainboard PRIVATE hwinfo::utils::wmi)

  install(TARGETS hwinfo_mainboard DESTINATION ${CMAKE_INSTALL_LIBDIR})

  target_link_libraries(hwinfo PUBLIC hwinfo_mainboard)
endif()

# os
if(HWINFO_OS)
  add_library(
    hwinfo_os
    os.cpp
    # apple specific definitions
    apple/os.cpp
    # linux specific definitions
    linux/os.cpp
    # windows specific definitions
    windows/os.cpp)
  add_library(hwinfo::os ALIAS hwinfo_os)

  target_include_directories(
    hwinfo_os PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                     $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_compile_definitions(hwinfo_os PUBLIC ${HWINFO_DEFINITIONS})

  target_link_libraries(hwinfo_os PRIVATE hwinfo::utils::wmi)

  target_link_libraries(hwinfo PUBLIC hwinfo_os)
  install(TARGETS hwinfo_os DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# ram
if(HWINFO_RAM)
  add_library(
    hwinfo_ram
    ram.cpp
    # apple specific definitions
    apple/ram.cpp
    # linux specific definitions
    linux/ram.cpp
    # windows specific definitions
    windows/ram.cpp)
  add_library(hwinfo::ram ALIAS hwinfo_ram)

  target_include_directories(
    hwinfo_ram PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>
                      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_compile_definitions(hwinfo_ram PUBLIC ${HWINFO_DEFINITIONS})

  target_link_libraries(hwinfo_ram PRIVATE hwinfo::utils::wmi)

  target_link_libraries(hwinfo PUBLIC hwinfo_ram)

  install(TARGETS hwinfo_ram DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()
