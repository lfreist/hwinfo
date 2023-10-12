add_library(HWinfo STATIC
        hwinfo.cpp
        # platform independent definitions
        PCIMapper.cpp
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
        windows/utils/wmi_wrapper.cpp
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

if (NOT DEFINED NO_OCL)
    target_link_libraries(HWinfo PUBLIC miss-opencl_static)
endif ()

add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)
