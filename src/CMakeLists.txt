# # ----- hwinfo ---------------------------------------------------------------------------------------------------------
file(GLOB_RECURSE HWINFO_SRC_FILES
    CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
)
list(FILTER HWINFO_SRC_FILES EXCLUDE REGEX ".*/opencl/.*")

add_library(hwinfo ${HWINFO_BUILD} ${HWINFO_SRC_FILES})
target_include_directories(hwinfo PUBLIC ${HWINFO_INCLUDE_DIR})

if(APPLE)
    target_link_libraries(hwinfo PRIVATE "-framework IOKit" "-framework CoreFoundation")
endif()

if (HWINFO_GPU_OPENCL)
    add_subdirectory(opencl)
    target_compile_definitions(hwinfo_gpu PUBLIC USE_OCL)
    target_link_libraries(hwinfo_gpu PRIVATE opencl_device)
endif ()
