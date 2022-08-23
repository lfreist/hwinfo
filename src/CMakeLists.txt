add_library(HWinfo
        cpu.cpp
        gpu.cpp
        ram.cpp
        os.cpp
        mainboard.cpp
        )

add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)