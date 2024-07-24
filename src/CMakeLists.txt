# ----- hwinfo ---------------------------------------------------------------------------------------------------------
add_library(hwinfo INTERFACE)
target_include_directories(hwinfo INTERFACE $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
add_library(hwinfo::hwinfo ALIAS hwinfo)

add_library(hwinfo_static INTERFACE)
target_include_directories(hwinfo_static INTERFACE $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
add_library(hwinfo::hwinfo_static ALIAS hwinfo_static)

# === Components =======================================================================================================
# ----- BATTERY --------------------------------------------------------------------------------------------------------
if (HWINFO_BATTERY)
    set(BATTERY_SRC_FILES
            battery.cpp
            apple/battery.cpp
            linux/battery.cpp
            windows/battery.cpp

            windows/utils/wmi_wrapper.cpp
    )

    add_library(hwinfo_battery SHARED ${BATTERY_SRC_FILES})
    target_compile_definitions(hwinfo_battery PUBLIC HWINFO_BATTERY -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_battery PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_battery_static STATIC ${BATTERY_SRC_FILES})
    target_compile_definitions(hwinfo_battery_static PUBLIC HWINFO_BATTERY)
    target_include_directories(hwinfo_battery_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_battery hwinfo_cpu hwinfo_disk hwinfo_gpu hwinfo_mainboard hwinfo_os hwinfo_ram)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_battery hwinfo_cpu hwinfo_disk hwinfo_gpu hwinfo_mainboard hwinfo_os hwinfo_ram)
endif ()
# ______________________________________________________________________________________________________________________

# ----- CPU ------------------------------------------------------------------------------------------------------------
if (HWINFO_CPU)
    set(CPU_SRC_FILES
            cpu.cpp
            apple/cpu.cpp
            linux/cpu.cpp
            windows/cpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_cpu SHARED ${CPU_SRC_FILES})
    target_compile_definitions(hwinfo_cpu PUBLIC HWINFO_CPU -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_cpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_cpu_static STATIC ${CPU_SRC_FILES})
    target_compile_definitions(hwinfo_cpu_static PUBLIC HWINFO_CPU)
    target_include_directories(hwinfo_cpu_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif()
# ______________________________________________________________________________________________________________________

# ----- DISK -----------------------------------------------------------------------------------------------------------
if (HWINFO_DISK)
    set(DISK_SRC_FILES
            disk.cpp
            apple/disk.cpp
            linux/disk.cpp
            windows/disk.cpp

            windows/utils/wmi_wrapper.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_disk SHARED ${DISK_SRC_FILES})
    target_compile_definitions(hwinfo_disk PUBLIC HWINFO_DISK -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_disk PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_disk_static STATIC ${DISK_SRC_FILES})
    target_compile_definitions(hwinfo_disk_static PUBLIC HWINFO_DISK)
    target_include_directories(hwinfo_disk_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif ()
# ______________________________________________________________________________________________________________________

# ----- GPU ------------------------------------------------------------------------------------------------------------
if (HWINFO_GPU)
    set(GPU_SRC_FILES
            gpu.cpp
            apple/gpu.cpp
            linux/gpu.cpp
            windows/gpu.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_gpu SHARED ${GPU_SRC_FILES})
    target_compile_definitions(hwinfo_gpu PUBLIC HWINFO_GPU -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_gpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_gpu_static STATIC ${GPU_SRC_FILES})
    target_compile_definitions(hwinfo_gpu_static PUBLIC HWINFO_GPU)
    target_include_directories(hwinfo_gpu_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif ()
# ______________________________________________________________________________________________________________________

# ----- Mainboard ------------------------------------------------------------------------------------------------------
if (HWINFO_MAINBOARD)
    set(MAINBOARD_SRC_FILES
            mainboard.cpp
            apple/mainboard.cpp
            linux/mainboard.cpp
            windows/mainboard.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_mainboard SHARED ${MAINBOARD_SRC_FILES})
    target_compile_definitions(hwinfo_mainboard PUBLIC HWINFO_MAINBOARD -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_mainboard PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_mainboard_static STATIC ${MAINBOARD_SRC_FILES})
    target_compile_definitions(hwinfo_mainboard_static PUBLIC HWINFO_MAINBOARD)
    target_include_directories(hwinfo_mainboard_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif ()
# ______________________________________________________________________________________________________________________

# ----- OS -------------------------------------------------------------------------------------------------------------
if (HWINFO_OS)
    set(OS_SRC_FILES
            os.cpp
            apple/os.cpp
            linux/os.cpp
            windows/os.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_os SHARED ${OS_SRC_FILES})
    target_compile_definitions(hwinfo_os PUBLIC HWINFO_OS -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_os PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_os_static STATIC ${OS_SRC_FILES})
    target_compile_definitions(hwinfo_os_static PUBLIC HWINFO_OS)
    target_include_directories(hwinfo_os_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif ()
# ______________________________________________________________________________________________________________________

# ----- RAM ------------------------------------------------------------------------------------------------------------
if (HWINFO_RAM)
    set(RAM_SRC_FILES
            ram.cpp
            apple/ram.cpp
            linux/ram.cpp
            windows/ram.cpp

            windows/utils/wmi_wrapper.cpp
            apple/utils/filesystem.cpp
            linux/utils/filesystem.cpp
    )

    add_library(hwinfo_ram SHARED ${RAM_SRC_FILES})
    target_compile_definitions(hwinfo_ram PUBLIC HWINFO_RAM -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_ram PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)

    add_library(hwinfo_ram_static STATIC ${RAM_SRC_FILES})
    target_compile_definitions(hwinfo_ram_static PUBLIC HWINFO_RAM)
    target_include_directories(hwinfo_ram_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}>)
endif ()
# ______________________________________________________________________________________________________________________