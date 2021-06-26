cmake_minimum_required(VERSION 3.5)

project(obinit-bin LANGUAGES C VERSION 0.1.0)

set(C_STANDARD 11)
set(TARGET obinit-bin)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

configure_file(version.h.in version.h)

add_executable(${TARGET}
  main.c
  ObArgParser.c
  ObYamlParser.c
  ObYamlConfigReader.c
  )
target_link_libraries(${TARGET} obinit yaml.a)
set_target_properties(${TARGET}
        PROPERTIES OUTPUT_NAME obinit)

