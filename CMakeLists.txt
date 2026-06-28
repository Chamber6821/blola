cmake_minimum_required(VERSION 3.31)
project(blola VERSION 0.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(${PROJECT_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(${PROJECT_NAME} PROPERTIES
  SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/blola.py")


if(BLOLA_EXAMPLES)

  # Configure example just_log

  set(EXAMPLE_PROJECT ${PROJECT_NAME}_example_just_log)
  add_executable(${EXAMPLE_PROJECT}
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/just_log/main.cpp")
  target_link_libraries(${EXAMPLE_PROJECT} PUBLIC ${PROJECT_NAME})
  set_target_properties(${EXAMPLE_PROJECT} PROPERTIES
  EXPORT_COMPILE_COMMANDS ON
  COMPILE_FLAGS "-save-temps=obj")

  # Configure example strings

  set(EXAMPLE_PROJECT ${PROJECT_NAME}_example_strings)
  add_executable(${EXAMPLE_PROJECT}
  "${CMAKE_CURRENT_SOURCE_DIR}/examples/strings/main.cpp")
  target_link_libraries(${EXAMPLE_PROJECT} PUBLIC ${PROJECT_NAME})
  set_target_properties(${EXAMPLE_PROJECT} PROPERTIES
  EXPORT_COMPILE_COMMANDS ON
  COMPILE_FLAGS "-save-temps=obj")

endif()
