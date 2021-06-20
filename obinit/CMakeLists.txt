cmake_minimum_required(VERSION 3.5)

project(obinit LANGUAGES C)

set(C_STANDARD 11)

add_subdirectory(libobinit)
add_subdirectory(src)

enable_testing()
add_subdirectory(tests/unit)
add_subdirectory(tests/integration)

