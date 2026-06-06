cmake_minimum_required(VERSION 3.24)

project(
  Speed
  VERSION 0.1.0
  DESCRIPTION "Independent browser and browser engine"
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(SPEED_ENABLE_TESTS "Build Speed tests" ON)
option(SPEED_ENABLE_STRICT_WARNINGS "Enable strict compiler warnings for Speed targets" ON)
option(SPEED_WARNINGS_AS_ERRORS "Treat compiler warnings as errors for Speed targets" ON)
option(SPEED_ENABLE_CLANG_TIDY "Run clang-tidy on Speed-owned C++ targets" OFF)

if(SPEED_ENABLE_CLANG_TIDY)
  find_program(SPEED_CLANG_TIDY_EXE NAMES clang-tidy REQUIRED)
  set(SPEED_CLANG_TIDY_COMMAND
      ${PROJECT_SOURCE_DIR}/tools/lint/clang-tidy-filter.sh
      --clang-tidy-binary
      ${SPEED_CLANG_TIDY_EXE}
      --extra-arg-before=-Wno-unknown-warning-option
      --config-file=${PROJECT_SOURCE_DIR}/.clang-tidy
      --header-filter=^${PROJECT_SOURCE_DIR}/\(src\|tests\)/
  )
endif()

include(cmake/SpeedCompilerOptions.cmake)

add_subdirectory(src)

if(SPEED_ENABLE_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
