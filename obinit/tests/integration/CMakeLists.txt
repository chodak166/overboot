cmake_minimum_required(VERSION 3.5)

set(UNITY_DIR ../../extern/unity/src)

include_directories(
    ${UNITY_DIR}
    )


file(COPY test_root DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY layers DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

set(UNITY_SRC ${UNITY_DIR}/unity.c)

add_executable(ObMountTest ${UNITY_SRC}
    ObMount.test.c
    ObMount.test_Runner.c
    )

#target_link_libraries(ObMountsTest linux)

add_test(ObMountTest ObMountTest)
