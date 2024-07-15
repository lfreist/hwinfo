add_executable(Example example.cpp)

include(FetchContent)
FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.1)
FetchContent_MakeAvailable(fmt)

target_link_libraries(Example PRIVATE hwinfo::hwinfo fmt::fmt)
