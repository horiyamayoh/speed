add_executable(speed-browser speed_browser_main.cpp)
target_link_libraries(speed-browser PRIVATE speed_browser)
speed_configure_target(speed-browser)

add_executable(speed-renderer speed_renderer_main.cpp)
target_link_libraries(speed-renderer PRIVATE speed_renderer)
speed_configure_target(speed-renderer)

add_executable(speed-network speed_network_main.cpp)
target_link_libraries(speed-network PRIVATE speed_network)
speed_configure_target(speed-network)

add_executable(speed-utility speed_utility_main.cpp)
target_link_libraries(speed-utility PRIVATE speed_utility)
speed_configure_target(speed-utility)

