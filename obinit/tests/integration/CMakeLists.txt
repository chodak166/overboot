cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)

include_directories(
    ${UNITY_DIR}
    )


file(COPY test_root DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY mount_layers DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(COMMON_SRC
  ObTestHelpers.c
  ${UNITY_DIR}/unity.c
  )

set(TEST_TARGET ObMountTest)
add_executable(${TEST_TARGET} ${COMMON_SRC}
    ObMount.test.c
    ObMount.test_Runner.c
    )
target_link_libraries(${TEST_TARGET} obinit)
add_test(${TEST_TARGET} ${TEST_TARGET})


set(TEST_TARGET ObInitTest)
add_executable(${TEST_TARGET} ${COMMON_SRC}
    ObInit.test.c
    ObInit.test_Runner.c
    )
add_test(${TEST_TARGET} ${TEST_TARGET})
target_link_libraries(${TEST_TARGET} obinit)
