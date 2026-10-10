include(FetchContent)
find_package(GTest QUIET)
if (NOT GTest_FOUND)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(googletest)
endif ()
include(GoogleTest)

add_executable(hwinfo_tests
        apple_models_test.cpp
        dmi_test.cpp
        error_test.cpp
        units_test.cpp
        strings_test.cpp
        procfs_test.cpp
        gpu_test.cpp
        gpu_merge_test.cpp
        ${PROJECT_SOURCE_DIR}/src/common/gpu/intel.cpp
        ${PROJECT_SOURCE_DIR}/src/common/gpu/merge.cpp
        ${PROJECT_SOURCE_DIR}/src/common/gpu/nvidia.cpp
        pci_test.cpp
        monitor_test.cpp
        smoke_test.cpp
)
target_include_directories(hwinfo_tests PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hwinfo_tests PRIVATE lfreist-hwinfo::hwinfo GTest::gtest_main)
foreach (component battery computer cpu disk gpu mainboard os ram network)
    if (TARGET hwinfo_${component})
        string(TOUPPER ${component} COMPONENT)
        target_compile_definitions(hwinfo_tests PRIVATE HWINFO_HAS_${COMPONENT})
    endif ()
endforeach ()
gtest_discover_tests(hwinfo_tests DISCOVERY_MODE PRE_TEST)
