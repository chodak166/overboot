cmake_minimum_required(VERSION 3.5)

project(overboot LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

get_filename_component(OB_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/cmake ABSOLUTE)
get_filename_component(OB_OBINIT_DIR ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)

option(OB_BUILD_OBINIT "Build obinit binary" ON)
if (${OB_BUILD_OBINIT})
  add_subdirectory(obinit)
endif()
