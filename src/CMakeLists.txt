add_library(speed_base
  base/logging/logging.cpp
  base/logging/logging.h
  base/result/status.h
  base/ids/id_types.h
)
target_include_directories(speed_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_base)

function(speed_assert_target_not_linking target_name)
  set(all_links)
  foreach(link_property LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
    get_target_property(links ${target_name} ${link_property})
    if(links)
      list(APPEND all_links ${links})
    endif()
  endforeach()

  foreach(forbidden_target IN LISTS ARGN)
    if(forbidden_target IN_LIST all_links)
      message(FATAL_ERROR
        "${target_name} must not link ${forbidden_target}; this would violate Speed process authority boundaries")
    endif()
  endforeach()
endfunction()

add_library(speed_ipc
  ipc/runtime/codec.cpp
  ipc/runtime/codec.h
  ipc/runtime/framed_transport.cpp
  ipc/runtime/framed_transport.h
  ipc/runtime/message.cpp
  ipc/runtime/message.h
  ipc/runtime/navigation_codec.cpp
  ipc/runtime/navigation_codec.h
  ipc/runtime/navigation_messages.cpp
  ipc/runtime/navigation_messages.h
)
target_link_libraries(speed_ipc PUBLIC speed_base)
target_include_directories(speed_ipc PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_ipc)

add_library(speed_platform
  platform/process/process_launcher.cpp
  platform/process/process_launcher.h
)
target_link_libraries(speed_platform PUBLIC speed_base)
target_include_directories(speed_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_platform)

find_package(X11 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED IMPORTED_TARGET cairo)

add_library(speed_platform_window
  platform/window/canvas.h
  platform/window/window.h
  platform/window/x11_window.cpp
)
target_link_libraries(speed_platform_window
  PUBLIC speed_base
  PRIVATE X11::X11 PkgConfig::CAIRO
)
target_include_directories(speed_platform_window PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_platform_window)

add_library(speed_aegis
  aegis/classifier/request_classifier.cpp
  aegis/classifier/request_classifier.h
  aegis/rules/rule.h
)
target_link_libraries(speed_aegis PUBLIC speed_base)
target_include_directories(speed_aegis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_aegis)

find_package(OpenSSL REQUIRED)

add_library(speed_storage
  storage/history/history_store.cpp
  storage/history/history_store.h
  storage/profile/profile_directory.cpp
  storage/profile/profile_directory.h
)
target_link_libraries(speed_storage PUBLIC speed_base)
target_include_directories(speed_storage PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_storage)

add_library(speed_engine
  engine/css/css_parser.cpp
  engine/css/css_parser.h
  engine/dom/node.h
  engine/html/html_parser.cpp
  engine/html/html_parser.h
  engine/layout/layout_engine.cpp
  engine/layout/layout_engine.h
  engine/paint/display_list.cpp
  engine/paint/display_list.h
  engine/render_pipeline.cpp
  engine/render_pipeline.h
  engine/style/style_resolver.cpp
  engine/style/style_resolver.h
)
target_link_libraries(speed_engine PUBLIC speed_base)
target_include_directories(speed_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_engine)

add_library(speed_ui
  ui/shell/browser_shell.cpp
  ui/shell/browser_shell.h
)
target_link_libraries(speed_ui PUBLIC speed_base speed_engine speed_platform)
target_include_directories(speed_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_ui)

add_library(speed_ui_gui
  ui/gui/browser_gui_shell.cpp
  ui/gui/browser_gui_shell.h
  ui/gui/display_list_renderer.cpp
  ui/gui/display_list_renderer.h
)
target_link_libraries(speed_ui_gui PUBLIC speed_base speed_engine speed_platform_window speed_ui)
target_include_directories(speed_ui_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_ui_gui)

add_library(speed_network
  network/fetch/fetch_adapter.h
  network/fetch/network_service.cpp
  network/fetch/network_service.h
  network/network_ipc_server.cpp
  network/network_ipc_server.h
  network/network_process.cpp
  network/network_process.h
)
target_link_libraries(speed_network PUBLIC speed_aegis speed_base speed_ipc)
target_link_libraries(speed_network PRIVATE OpenSSL::SSL)
target_include_directories(speed_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_network)

add_library(speed_renderer
  renderer/document/renderer_ipc_server.cpp
  renderer/document/renderer_ipc_server.h
  renderer/document/renderer_process.cpp
  renderer/document/renderer_process.h
)
target_link_libraries(speed_renderer PUBLIC speed_base speed_engine speed_ipc speed_platform)
target_include_directories(speed_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_renderer)
speed_assert_target_not_linking(speed_renderer speed_network speed_storage)

add_library(speed_browser
  browser/browser_process.cpp
  browser/browser_process.h
  browser/navigation/navigation_controller.cpp
  browser/navigation/navigation_controller.h
  browser/process_host/child_process_host.cpp
  browser/process_host/child_process_host.h
  browser/tabs/tab_model.cpp
  browser/tabs/tab_model.h
)
target_link_libraries(speed_browser PUBLIC speed_base speed_ipc speed_platform speed_storage)
target_include_directories(speed_browser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_browser)
speed_assert_target_not_linking(speed_browser speed_network speed_renderer)

add_library(speed_utility
  utility/utility_process.cpp
  utility/utility_process.h
)
target_link_libraries(speed_utility PUBLIC speed_base speed_ipc)
target_include_directories(speed_utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_utility)

add_subdirectory(app)
