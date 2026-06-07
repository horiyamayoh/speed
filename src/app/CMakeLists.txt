add_library(speed_app_core
  browser_app.cpp
  browser_app.h
)
target_link_libraries(
  speed_app_core
  PUBLIC speed_browser speed_network speed_renderer speed_ui
)
target_include_directories(speed_app_core PUBLIC ${PROJECT_SOURCE_DIR}/src)
speed_configure_target(speed_app_core)

add_executable(speed-browser speed_browser_main.cpp)
target_link_libraries(speed-browser PRIVATE speed_app_core)
speed_configure_target(speed-browser)

add_executable(speed-browser-gui speed_browser_gui_main.cpp)
target_link_libraries(speed-browser-gui PRIVATE speed_app_core speed_ui_gui)
speed_configure_target(speed-browser-gui)

add_executable(speed-renderer speed_renderer_main.cpp)
target_link_libraries(speed-renderer PRIVATE speed_renderer)
speed_configure_target(speed-renderer)

add_executable(speed-network speed_network_main.cpp)
target_link_libraries(speed-network PRIVATE speed_network)
speed_configure_target(speed-network)

add_executable(speed-utility speed_utility_main.cpp)
target_link_libraries(speed-utility PRIVATE speed_utility)
speed_configure_target(speed-utility)
