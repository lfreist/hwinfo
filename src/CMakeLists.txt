add_library(HWinfo
        # platform independent definitions
        battery.cpp
        cpu.cpp
        gpu.cpp
        ram.cpp
        os.cpp
        mainboard.cpp
        disk.cpp
        # apple specific definitions
        apple/utils/filesystem.cpp
        apple/battery.cpp
        apple/cpu.cpp
        apple/gpu.cpp
        apple/ram.cpp
        apple/os.cpp
        apple/mainboard.cpp
        apple/disk.cpp
        # linux specific definitions
        linux/utils/filesystem.cpp
        linux/battery.cpp
        linux/cpu.cpp
        linux/gpu.cpp
        linux/ram.cpp
        linux/os.cpp
        linux/mainboard.cpp
        linux/disk.cpp
        # windows specific definitions
        windows/utils/filesystem.cpp
        windows/battery.cpp
        windows/cpu.cpp
        windows/gpu.cpp
        windows/ram.cpp
        windows/os.cpp
        windows/mainboard.cpp
        windows/disk.cpp
        )

if (WIN32 AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_link_libraries(HWinfo PUBLIC "wbemuuid")
endif ()

target_include_directories(HWinfo
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

set(HWinfo_public_headers
    "${PROJECT_SOURCE_DIR}/include/hwinfo/WMIwrapper.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/battery.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/cpu.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/cpuid.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/disk.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/gpu.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/hwinfo.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/mainboard.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/os.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/platform.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/ram.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/system.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/utils/filesystem.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/utils/stringutils.h"
    "${PROJECT_SOURCE_DIR}/include/hwinfo/utils/subprocess.h"
)

set_property(TARGET HWinfo 
  PROPERTY PUBLIC_HEADER ${HWinfo_public_headers}
)

add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS HWinfo
    EXPORT HWinfoTargets
    PUBLIC_HEADER DESTINATION include/hwinfo
)

install(EXPORT HWinfoTargets
  FILE HWinfoTargets.cmake
  DESTINATION lib/cmake/HWinfo
)

configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HWinfoConfig.cmake"
    INSTALL_DESTINATION "lib/cmake/hwinfo"
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/HWinfoConfigVersion.cmake"
    VERSION "${HWinfo_VERSION_MAJOR}.${HWinfo_VERSION_MINOR}"
    COMPATIBILITY AnyNewerVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HWinfoConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HWinfoConfigVersion.cmake"
    DESTINATION lib/cmake/HWinfo
)
