add_executable(system_info system_infoMain.cpp)

target_link_libraries(system_info PRIVATE lfreist-hwinfo::hwinfo)

install(TARGETS system_info)
