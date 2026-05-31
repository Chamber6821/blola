cmake_minimum_required(VERSION 3.31)
project(blola VERSION 0.0.0 LANGUAGES CXX)

add_library(${CMAKE_PROJECT_NAME} INTERFACE)

target_compile_features(${CMAKE_PROJECT_NAME} INTERFACE cxx_std_23)

target_include_directories(${CMAKE_PROJECT_NAME} INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}")

set(LOGGER_SCRIPT "./blola.py")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/${LOGGER_SCRIPT}"
  "${CMAKE_CURRENT_BINARY_DIR}/${LOGGER_SCRIPT}"
  COPYONLY
)

