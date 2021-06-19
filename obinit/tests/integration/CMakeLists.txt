cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)

include_directories(
    ${UNITY_DIR}
    )


file(COPY test_root DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY mount_layers DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(UNITY_SRC ${UNITY_DIR}/unity.c)

set(TEST_TARGET ObMountTest)
add_executable(${TEST_TARGET} ${UNITY_SRC}
    ObMount.test.c
    ObMount.test_Runner.c
    )
add_test(${TEST_TARGET} ${TEST_TARGET})


set(TEST_TARGET ObInitTest)
add_executable(${TEST_TARGET} ${UNITY_SRC}
    ObInit.test.c
    ObInit.test_Runner.c
    )
add_test(${TEST_TARGET} ${TEST_TARGET})
