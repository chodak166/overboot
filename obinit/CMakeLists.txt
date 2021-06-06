cmake_minimum_required(VERSION 3.5)

project(obinit LANGUAGES C)

add_executable(obinit main.c)

enable_testing()
add_subdirectory(tests/unit)

