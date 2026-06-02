cmake_minimum_required(VERSION 3.31)
project(blola VERSION 0.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} INTERFACE)

target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(${PROJECT_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}")

set_target_properties(${PROJECT_NAME} PROPERTIES
  SCRIPT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/blola.py")

