add_executable(system_info system_infoMain.cpp)
target_link_libraries(system_info PRIVATE lfreist-hwinfo::hwinfo)
install(TARGETS system_info)

add_executable(live_monitor live_monitorMain.cpp)
target_link_libraries(live_monitor PRIVATE lfreist-hwinfo::hwinfo)
install(TARGETS live_monitor)
