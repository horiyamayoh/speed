add_library(speed_base
  base/logging/logging.cpp
  base/logging/logging.h
  base/result/status.h
  base/ids/id_types.h
)
target_include_directories(speed_base PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_base)

add_library(speed_ipc
  ipc/runtime/message.cpp
  ipc/runtime/message.h
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

add_library(speed_aegis
  aegis/classifier/request_classifier.cpp
  aegis/classifier/request_classifier.h
  aegis/rules/rule.h
)
target_link_libraries(speed_aegis PUBLIC speed_base)
target_include_directories(speed_aegis PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_aegis)

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
  engine/dom/node.h
  engine/html/html_parser.cpp
  engine/html/html_parser.h
)
target_link_libraries(speed_engine PUBLIC speed_base)
target_include_directories(speed_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_engine)

add_library(speed_ui
  ui/shell/browser_shell.cpp
  ui/shell/browser_shell.h
)
target_link_libraries(speed_ui PUBLIC speed_base speed_platform)
target_include_directories(speed_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_ui)

add_library(speed_network
  network/fetch/network_service.cpp
  network/fetch/network_service.h
  network/network_process.cpp
  network/network_process.h
)
target_link_libraries(speed_network PUBLIC speed_aegis speed_base speed_ipc)
target_include_directories(speed_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_network)

add_library(speed_renderer
  renderer/document/renderer_process.cpp
  renderer/document/renderer_process.h
)
target_link_libraries(speed_renderer PUBLIC speed_base speed_engine speed_ipc speed_platform)
target_include_directories(speed_renderer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_renderer)

add_library(speed_browser
  browser/browser_process.cpp
  browser/browser_process.h
  browser/navigation/navigation_controller.cpp
  browser/navigation/navigation_controller.h
  browser/tabs/tab_model.cpp
  browser/tabs/tab_model.h
)
target_link_libraries(speed_browser PUBLIC speed_aegis speed_base speed_ipc speed_platform speed_storage speed_ui)
target_include_directories(speed_browser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_browser)

add_library(speed_utility
  utility/utility_process.cpp
  utility/utility_process.h
)
target_link_libraries(speed_utility PUBLIC speed_base speed_ipc)
target_include_directories(speed_utility PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
speed_configure_target(speed_utility)

add_subdirectory(app)

