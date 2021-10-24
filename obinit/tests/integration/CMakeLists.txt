cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)

include_directories(
  ${OBINIT_DIR}/libobinit/src
  ${UNITY_DIR}
  )

file(COPY test_root DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(COPY mount_layers DESTINATION ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

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

set(TEST_TARGET ObSyncTest)
add_executable(${TEST_TARGET} ${COMMON_SRC}
  ObSync.test.c
  ObSync.test_Runner.c
  )
target_link_libraries(${TEST_TARGET} obinit)
add_test(${TEST_TARGET} ${TEST_TARGET})
