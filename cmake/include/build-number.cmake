
string(TOLOWER "${CMAKE_BUILD_TYPE}" LCASE_BUILD_TYPE)

set(BUILD_NUMBER 0)

if ( (NOT "${LCASE_BUILD_TYPE}" STREQUAL "debug") AND (NOT "${CMAKE_BUILD_TYPE}" STREQUAL ""))

  set (BUILD_NUM_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/increment-build-number.cmake")
  if (EXISTS "${BUILD_NUM_SCRIPT}")
      set (BUILD_NUM_FILE "${CMAKE_CURRENT_SOURCE_DIR}/build_number")
      add_custom_target (build_num_target_${PROJECT_NAME} ALL DEPENDS build_num)

      add_custom_command (OUTPUT build_num COMMAND
          ${CMAKE_COMMAND} -DBUILD_NUM_FILE="${BUILD_NUM_FILE}" -P "${BUILD_NUM_SCRIPT}")

      if (NOT EXISTS ${BUILD_NUM_FILE})    
        execute_process(COMMAND ${CMAKE_COMMAND} -DBUILD_NUM_FILE=${BUILD_NUM_FILE} -P ${BUILD_NUM_SCRIPT} COMMAND_ECHO STDOUT)
      endif()
      
      file(STRINGS "${BUILD_NUM_FILE}" BUILD_NUMBER)
      MESSAGE(STATUS "Incremented build number: ${BUILD_NUMBER}")
  else()
    MESSAGE("${BUILD_NUM_SCRIPT} not found")
  endif()

endif()
