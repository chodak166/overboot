cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)

include_directories(
    ${UNITY_DIR}
    )

set(UNITY_SRC ${UNITY_DIR}/unity.c)

add_executable(TaskListTest ${UNITY_SRC}
    TaskList.test.c
    TaskList.test_Runner.c
    )
add_test(TaskListTest TaskListTest)
