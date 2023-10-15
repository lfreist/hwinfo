# Define the minimum required CMake version to run this file. 
cmake_minimum_required(VERSION 3.16)

# Define the project and its name, version, and specified languages.
project(hwinfo VERSION 1.0.0 LANGUAGES CXX)

# Set the C++ language standard to be used. 
# If NO_OCL is defined then C++11 standard will be used, else it will be C++20.
if (NOT DEFINED NO_OCL)
    set(CMAKE_CXX_STANDARD 20)
else ()
    set(CMAKE_CXX_STANDARD 11)
endif ()

# Ensure that the standard requirement is mandatory.
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define the output path for libraries and archives.
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Check if the current source directory (where this CMakeLists.txt is located)
# is the root of the source tree, then this is the MAIN_PROJECT.
set(MAIN_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(MAIN_PROJECT ON)
endif()

# If NO_OCL is not defined, include OpenCL related configuration.
if (NOT DEFINED NO_OCL)
    # Don't build OpenCL SDK samples
    set(OPENCL_SDK_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
    # Specify additional directories to search for header files.
    include_directories(external/miss-opencl/include)
    include_directories(external/miss-opencl/external/OpenCL/external/OpenCL-CLHPP/include)
    # If the static library for miss-opencl is not yet defined, add it.
    if (NOT TARGET miss-opencl_static)
        add_subdirectory(external/miss-opencl)
    endif ()
    # Add specific compile-time definitions.
    add_compile_definitions(USE_OCL)
    add_compile_definitions(NOMINMAX)
endif ()


add_library(HWinfo INTERFACE)
add_library(${PROJECT_NAME}::HWinfo ALIAS HWinfo)
target_include_directories(HWinfo INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>/include)

if (WIN32 AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    target_link_libraries(HWinfo PUBLIC "wbemuuid")
endif ()

if (NOT DEFINED NO_OCL)
    target_link_libraries(HWinfo PUBLIC miss-opencl_static)
endif ()

# If the host system is Linux, copy the "pci.ids" to a destination folder.
if (CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    file(COPY "scripts/pci.ids" DESTINATION "$ENV{HOME}/.hwinfo/")
endif ()

# If this is the MAIN_PROJECT, it will include the examples and the tests.
if (${MAIN_PROJECT})
    # Add the examples subdirectory.
    add_subdirectory(examples)
    # Enable testing and add the test directory.
    include(CTest)
    add_subdirectory(test)
    # Define a test named "Example"
    add_test(Example examples/Example)
endif()