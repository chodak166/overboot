cmake_minimum_required(VERSION 3.5)

project(obinit-bin LANGUAGES C)

set(C_STANDARD 11)
set(TARGET obinit-bin)

add_executable(${TARGET} main.c)
target_link_libraries(${TARGET} obinit)
set_target_properties(${TARGET}
        PROPERTIES OUTPUT_NAME obinit)

