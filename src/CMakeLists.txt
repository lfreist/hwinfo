#add_library(HWinfo STATIC
#        # apple specific definitions
#        ../include/hwinfo/apple/utils/filesystem.h
#        ../include/hwinfo/apple/battery.h
#        ../include/hwinfo/apple/cpu.h
#        ../include/hwinfo/apple/gpu.h
#        ../include/hwinfo/apple/ram.h
#        ../include/hwinfo/apple/os.h
#        ../include/hwinfo/apple/mainboard.h
#        ../include/hwinfo/apple/disk.h
#        # linux specific definitions
#        ../include/hwinfo/linux/utils/filesystem.h
#        ../include/hwinfo/linux/battery.h
#        ../include/hwinfo/linux/cpu.h
#        ../include/hwinfo/linux/gpu.h
#        ../include/hwinfo/linux/ram.h
#        ../include/hwinfo/linux/os.h
#        ../include/hwinfo/linux/mainboard.h
#        ../include/hwinfo/linux/disk.h
#        # windows specific definitions
#        ../include/hwinfo/windows/utils/filesystem.h
#        ../include/hwinfo/windows/utils/wmi_wrapper.h
#        ../include/hwinfo/windows/battery.h
#        ../include/hwinfo/windows/cpu.h
#        ../include/hwinfo/windows/gpu.h
#        ../include/hwinfo/windows/ram.h
#        ../include/hwinfo/windows/os.h
#        ../include/hwinfo/windows/mainboard.h
#        ../include/hwinfo/windows/disk.h
#        )

#if (WIN32 AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
#    target_link_libraries(HWinfo PUBLIC "wbemuuid")
#endif ()
#
#if (NOT DEFINED NO_OCL)
#    target_link_libraries(HWinfo PUBLIC miss-opencl_static)
#endif ()

#add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)
