add_executable(wmitest test.cpp)
target_include_directories(wmitest PUBLIC ${PROJECT_SOURCE_DIR}/include/)
target_link_libraries(wmitest PUBLIC hwinfo::HWinfo)