include(FetchContent)
FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 11.0.1)
FetchContent_MakeAvailable(fmt)

add_executable(system_infoMain system_infoMain.cpp)

target_link_libraries(system_infoMain PRIVATE hwinfo::hwinfo fmt::fmt)
