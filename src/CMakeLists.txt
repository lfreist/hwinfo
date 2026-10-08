# ----- hwinfo ---------------------------------------------------------------------------------------------------------
add_library(hwinfo INTERFACE)
target_include_directories(hwinfo INTERFACE $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(hwinfo INTERFACE cxx_std_23)
add_library(lfreist-hwinfo::hwinfo ALIAS hwinfo)
set_target_properties(hwinfo PROPERTIES OUTPUT_NAME "hwinfo")

if(WIN32)
    set(HWINFO_PLATFORM windows)
elseif(APPLE)
    set(HWINFO_PLATFORM apple)
else()
    set(HWINFO_PLATFORM linux)
endif()

set(HWINFO_PUBLIC_HEADERS
        ${HWINFO_INCLUDE_DIR}/hwinfo/platform.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/error.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/units.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/monitoring.h
        ${HWINFO_INCLUDE_DIR}/hwinfo/hwinfo.h
)

# hwinfo_add_component(<name> SOURCES <files...> [PUBLIC_LINK <libs...>] [PRIVATE_LINK <libs...>])
# Sources are relative to src/; <platform>/<name>.cpp is added automatically.
function(hwinfo_add_component name)
    cmake_parse_arguments(ARG "" "" "SOURCES;PUBLIC_LINK;PRIVATE_LINK" ${ARGN})
    set(target hwinfo_${name})
    add_library(${target} ${HWINFO_PLATFORM}/${name}.cpp ${ARG_SOURCES})
    add_library(lfreist-hwinfo::${name} ALIAS ${target})
    target_compile_features(${target} PUBLIC cxx_std_23)
    target_include_directories(${target}
            PUBLIC $<BUILD_INTERFACE:${HWINFO_INCLUDE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
            PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    if(WIN32)
        if(BUILD_SHARED_LIBS)
            target_compile_definitions(${target} PRIVATE HWINFO_EXPORTS)
        else()
            target_compile_definitions(${target} PUBLIC HWINFO_STATIC)
        endif()
        target_link_libraries(${target} PRIVATE wbemuuid ole32 oleaut32)
    endif()
    if(APPLE)
        target_link_libraries(${target} PRIVATE "-framework IOKit" "-framework CoreFoundation")
    endif()
    target_link_libraries(${target} PUBLIC ${ARG_PUBLIC_LINK} PRIVATE ${ARG_PRIVATE_LINK})
    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${target} EXPORT_NAME ${name})
    target_link_libraries(hwinfo INTERFACE ${target})

    install(TARGETS ${target}
            EXPORT lfreist-hwinfoTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(FILES ${HWINFO_INCLUDE_DIR}/hwinfo/${name}.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
endfunction()

if(WIN32)
    set(HWINFO_WMI_SOURCES windows/wmi_wrapper.cpp)
endif()

# === Components =======================================================================================================
if (HWINFO_BATTERY)
    hwinfo_add_component(battery SOURCES ${HWINFO_WMI_SOURCES})
endif ()

if (HWINFO_CPU)
    hwinfo_add_component(cpu
            SOURCES common/cpu_sampler.cpp ${HWINFO_PLATFORM}/monitoring/cpu.cpp ${HWINFO_WMI_SOURCES}
            PRIVATE_LINK $<$<BOOL:${WIN32}>:ntdll> $<$<BOOL:${WIN32}>:powrprof>)
endif()

if (HWINFO_DISK)
    hwinfo_add_component(disk SOURCES ${HWINFO_WMI_SOURCES})
endif ()

if (HWINFO_GPU)
    hwinfo_add_component(gpu
            SOURCES ${HWINFO_WMI_SOURCES}
            PRIVATE_LINK $<$<BOOL:${WIN32}>:dxgi>)
    target_include_directories(hwinfo_gpu PRIVATE ${HWINFO_CMAKE_BINARY_DIR}/generated)
    add_dependencies(hwinfo_gpu generate_pci_ids_header)
    if (HWINFO_GPU_OPENCL)
        add_subdirectory(opencl)
        target_compile_definitions(hwinfo_gpu PRIVATE USE_OCL)
        target_link_libraries(hwinfo_gpu PRIVATE opencl_device)
    endif ()
endif ()

if (HWINFO_MAINBOARD)
    hwinfo_add_component(mainboard SOURCES ${HWINFO_WMI_SOURCES})
endif ()

if (HWINFO_OS)
    hwinfo_add_component(os SOURCES ${HWINFO_WMI_SOURCES})
endif ()

if (HWINFO_RAM)
    hwinfo_add_component(ram SOURCES ${HWINFO_PLATFORM}/monitoring/ram.cpp ${HWINFO_WMI_SOURCES})
endif ()

if (HWINFO_NETWORK)
    hwinfo_add_component(network
            PRIVATE_LINK $<$<BOOL:${WIN32}>:iphlpapi> $<$<BOOL:${WIN32}>:ws2_32>)
endif ()
# ______________________________________________________________________________________________________________________

install(FILES ${HWINFO_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
install(DIRECTORY ${HWINFO_INCLUDE_DIR}/hwinfo/detail DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/hwinfo)
install(TARGETS hwinfo
        EXPORT lfreist-hwinfoTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
