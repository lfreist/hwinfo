message("Using OpenCL for GPU information...")

find_package(OpenCLHeaders CONFIG)
if (OpenCLHeaders_FOUND)
    find_package(OpenCLHeadersCpp REQUIRED)
    find_package(OpenCLICDLoader REQUIRED)
else()
    set(OPENCL_SDK_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    include(FetchContent)

    FetchContent_Declare(
            opencl
            GIT_REPOSITORY https://github.com/KhronosGroup/OpenCL-SDK.git
            GIT_TAG v2023.04.17
    )
    FetchContent_MakeAvailable(opencl)
endif()

add_library(opencl_device STATIC device.cpp)
target_include_directories(opencl_device PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(opencl_device PUBLIC OpenCL::HeadersCpp OpenCL::OpenCL)