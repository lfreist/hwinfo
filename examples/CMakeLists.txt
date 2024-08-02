find_package(fmt QUIET)

if (NOT fmt_FOUND)
    message(" -> libfmt not found. Fetching from github...")
    include(FetchContent)
    FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.1)
    FetchContent_MakeAvailable(fmt)
else ()
    message(" -> using installed libfmt")
endif ()

add_executable(system_info system_infoMain.cpp)
add_executable(system_info_stand_alone system_infoMain.cpp)

target_link_libraries(system_info PRIVATE hwinfo::hwinfo fmt::fmt)
target_link_libraries(system_info_stand_alone PRIVATE hwinfo::hwinfo_static fmt::fmt)

install(TARGETS system_info system_info_stand_alone)