add_library(HWinfo
        # platform independent definitions
        cpu.cpp
        gpu.cpp
        ram.cpp
        os.cpp
        mainboard.cpp
        disk.cpp
        # apple specific definitions
        apple/cpu.cpp
        apple/gpu.cpp
        apple/ram.cpp
        apple/os.cpp
        apple/mainboard.cpp
        apple/disk.cpp
        # linux specific definitions
        linux/cpu.cpp
        linux/gpu.cpp
        linux/ram.cpp
        linux/os.cpp
        linux/mainboard.cpp
        linux/disk.cpp
        # windows specific definitions
        windows/cpu.cpp
        windows/gpu.cpp
        windows/ram.cpp
        windows/os.cpp
        windows/mainboard.cpp
        windows/disk.cpp
        )

add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)