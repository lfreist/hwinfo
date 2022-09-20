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
        apple/battery.cpp
        apple/cpu.cpp
        apple/gpu.cpp
        apple/ram.cpp
        apple/os.cpp
        apple/mainboard.cpp
        apple/disk.cpp
        # linux specific definitions
        linux/battery.cpp
        linux/cpu.cpp
        linux/gpu.cpp
        linux/ram.cpp
        linux/os.cpp
        linux/mainboard.cpp
        linux/disk.cpp
        # windows specific definitions
        windows/battery.cpp
        windows/cpu.cpp
        windows/gpu.cpp
        windows/ram.cpp
        windows/os.cpp
        windows/mainboard.cpp
        windows/disk.cpp
        )

add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)