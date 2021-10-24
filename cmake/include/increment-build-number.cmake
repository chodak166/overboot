set (BUILD_NUM_FILE "" CACHE STRING "Build number value file")

message (STATUS "Incrementing build number")

if (EXISTS ${BUILD_NUM_FILE})
  message (STATUS "build number file found")

  file(READ ${BUILD_NUM_FILE} BUILD_NUMBER)
  math(EXPR BUILD_NUMBER ${BUILD_NUMBER}+1)
else ()
  message (STATUS "build number file not found")
  set(BUILD_NUMBER 1)
endif ()
  
message (STATUS "writing build number file: ${BUILD_NUM_FILE} ${BUILD_NUMBER}")
file (WRITE ${BUILD_NUM_FILE} ${BUILD_NUMBER})
