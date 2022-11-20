cmake_minimum_required(VERSION 3.5)

project(obinit LANGUAGES C)

set(C_STANDARD 11)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

get_filename_component(OB_CMAKE_DIR ${CMAKE_CURRENT_LIST_DIR}/../cmake ABSOLUTE)
get_filename_component(OB_OBINIT_DIR ${CMAKE_CURRENT_LIST_DIR} ABSOLUTE)

add_subdirectory(lib)
add_subdirectory(apps/obinit)

option(OB_BUILD_TESTS "Build tests" ON)
if (${OB_BUILD_TESTS})
  enable_testing()
  add_subdirectory(tests/unit)
  add_subdirectory(tests/integration)
endif()
