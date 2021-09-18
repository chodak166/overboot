cmake_minimum_required(VERSION 3.5)

project(libobinit LANGUAGES C)

set(C_STANDARD 11)

set(TARGET obinit)
add_library(${TARGET} STATIC
  src/ObOsUtils.c
  src/ObMount.c
  src/ObContext.c
  src/ObLogging.c
  src/ObYamlParser.c
  src/ObYamlConfigReader.c
  src/ObYamlLayerReader.c
  )

target_include_directories(${TARGET}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include
  )
