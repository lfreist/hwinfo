# ----- hwinfo ---------------------------------------------------------------------------------------------------------
add_library(hwinfo INTERFACE)
target_include_directories(hwinfo INTERFACE $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
add_library(hwinfo::hwinfo ALIAS hwinfo)
set_target_properties(hwinfo PROPERTIES OUTPUT_NAME "hwinfo")

add_library(hwinfo_static INTERFACE)
target_include_directories(hwinfo_static INTERFACE $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
add_library(hwinfo::hwinfo_static ALIAS hwinfo_static)
set_target_properties(hwinfo_static PROPERTIES OUTPUT_NAME "hwinfo")

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
    target_compile_definitions(hwinfo_battery PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_battery PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(APPLE)
        target_link_libraries(hwinfo_battery PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()

    add_library(hwinfo_battery_static STATIC ${BATTERY_SRC_FILES})
    target_include_directories(hwinfo_battery_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(APPLE)
        target_link_libraries(hwinfo_battery_static PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()

    target_link_libraries(hwinfo INTERFACE hwinfo_battery)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_battery_static)

    set_target_properties(hwinfo_battery PROPERTIES OUTPUT_NAME "hwinfo_battery")
    set_target_properties(hwinfo_battery_static PROPERTIES OUTPUT_NAME "hwinfo_battery")

    install(TARGETS hwinfo_battery hwinfo_battery_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/battery.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
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
    target_compile_definitions(hwinfo_cpu PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_cpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(hwinfo_cpu_static STATIC ${CPU_SRC_FILES})
    target_include_directories(hwinfo_cpu_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_cpu)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_cpu_static)

    set_target_properties(hwinfo_cpu PROPERTIES OUTPUT_NAME "hwinfo_cpu")
    set_target_properties(hwinfo_cpu_static PROPERTIES OUTPUT_NAME "hwinfo_cpu")

    install(TARGETS hwinfo_cpu hwinfo_cpu_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/cpu.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
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
    target_compile_definitions(hwinfo_disk PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_disk PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(APPLE)
        target_link_libraries(hwinfo_disk PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()

    add_library(hwinfo_disk_static STATIC ${DISK_SRC_FILES})
    target_include_directories(hwinfo_disk_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    if(APPLE)
        target_link_libraries(hwinfo_disk_static PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()

    target_link_libraries(hwinfo INTERFACE hwinfo_disk)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_disk_static)

    set_target_properties(hwinfo_disk PROPERTIES OUTPUT_NAME "hwinfo_disk")
    set_target_properties(hwinfo_disk_static PROPERTIES OUTPUT_NAME "hwinfo_disk")

    install(TARGETS hwinfo_disk hwinfo_disk_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/disk.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
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

            PCIMapper.cpp
    )

    if (HWINFO_GPU_OPENCL)
        add_subdirectory(opencl)
    endif ()

    add_library(hwinfo_gpu SHARED ${GPU_SRC_FILES})
    target_compile_definitions(hwinfo_gpu PUBLIC -DHWINFO_EXPORTS)
    if(HWINFO_GPU_OPENCL)
        target_compile_definitions(hwinfo_gpu PUBLIC USE_OCL)
        target_link_libraries(hwinfo_gpu PRIVATE opencl_device)
    endif ()
    target_include_directories(hwinfo_gpu PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(hwinfo_gpu_static STATIC ${GPU_SRC_FILES})
    if(HWINFO_GPU_OPENCL)
        target_compile_definitions(hwinfo_gpu_static PUBLIC USE_OCL)
        target_link_libraries(hwinfo_gpu_static PRIVATE opencl_device)
    endif ()
    target_include_directories(hwinfo_gpu_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_gpu)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_gpu_static)

    set_target_properties(hwinfo_gpu PROPERTIES OUTPUT_NAME "hwinfo_gpu")
    set_target_properties(hwinfo_gpu_static PROPERTIES OUTPUT_NAME "hwinfo_gpu")

    install(TARGETS hwinfo_gpu hwinfo_gpu_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/gpu.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
    if(HWINFO_GPU_OPENCL)
        install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/opencl DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
    endif ()
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
    target_compile_definitions(hwinfo_mainboard PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_mainboard PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(hwinfo_mainboard_static STATIC ${MAINBOARD_SRC_FILES})
    target_include_directories(hwinfo_mainboard_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_mainboard)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_mainboard_static)

    set_target_properties(hwinfo_mainboard PROPERTIES OUTPUT_NAME "hwinfo_mainboard")
    set_target_properties(hwinfo_mainboard_static PROPERTIES OUTPUT_NAME "hwinfo_mainboard")

    install(TARGETS hwinfo_mainboard hwinfo_mainboard_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/mainboard.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
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
    target_compile_definitions(hwinfo_os PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_os PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(hwinfo_os_static STATIC ${OS_SRC_FILES})
    target_include_directories(hwinfo_os_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_os)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_os_static)

    set_target_properties(hwinfo_os PROPERTIES OUTPUT_NAME "hwinfo_os")
    set_target_properties(hwinfo_os_static PROPERTIES OUTPUT_NAME "hwinfo_os")

    install(TARGETS hwinfo_os hwinfo_os_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/os.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
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
    target_compile_definitions(hwinfo_ram PUBLIC -DHWINFO_EXPORTS)
    target_include_directories(hwinfo_ram PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    add_library(hwinfo_ram_static STATIC ${RAM_SRC_FILES})
    target_include_directories(hwinfo_ram_static PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

    target_link_libraries(hwinfo INTERFACE hwinfo_ram)
    target_link_libraries(hwinfo_static INTERFACE hwinfo_ram_static)

    set_target_properties(hwinfo_ram PROPERTIES OUTPUT_NAME "hwinfo_ram")
    set_target_properties(hwinfo_ram_static PROPERTIES OUTPUT_NAME "hwinfo_ram")

    install(TARGETS hwinfo_ram hwinfo_ram_static
            EXPORT hwinfoTargets
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/ram.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
endif ()
# ______________________________________________________________________________________________________________________

install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/platform.h ${HWINFO_INCLUDE_DIR}/hwinfo/hwinfo.h  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/utils DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
install(TARGETS hwinfo hwinfo_static
        EXPORT hwinfoTargets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
)