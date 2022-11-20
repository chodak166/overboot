cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)
set(LIBOBINIT_DIR ../../lib)

include_directories(
    ${UNITY_DIR}
    )

set(UNITY_SRC ${UNITY_DIR}/unity.c)

add_executable(TaskListTest ${UNITY_SRC}
    ${LIBOBINIT_DIR}/src/ObTaskList.c
    TaskList.test.c
    TaskList.test_Runner.c
    )
 target_include_directories(TaskListTest PRIVATE ${LIBOBINIT_DIR}/src)

add_test(TaskListTest TaskListTest)
