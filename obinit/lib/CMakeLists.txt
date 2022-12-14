cmake_minimum_required(VERSION 3.5)

project(libobinit LANGUAGES C)

set(C_STANDARD 11)

set(TARGET obinit)
add_library(${TARGET} STATIC
  src/ObOsUtils.c
  src/ObMount.c
  src/ObContext.c
  src/ObLogging.c
  src/ObYamlParser.c
  src/ObYamlConfigReader.c
  src/ObYamlLayerReader.c
  src/ObConfig.c
  src/ObInit.c
  src/ObFstab.c
  src/ObLayerCollector.c
  src/ObDeinit.c
  src/ObTaskList.c
  src/ObInitTasks.c
  src/ObPaths.c
  src/ObBlkid.c
  src/ObJobs.c
  src/ObXxHash.c

  extern/sds/sds.c
  extern/xxHash/xxhash.c
  )

target_include_directories(${TARGET}
  PUBLIC
  ${CMAKE_CURRENT_LIST_DIR}/include

  PRIVATE
  src
  ${CMAKE_CURRENT_LIST_DIR}/extern/sds
  ${CMAKE_CURRENT_LIST_DIR}/extern/xxHash
  )

target_compile_definitions(${TARGET}
  PRIVATE
  -D_GNU_SOURCE

  #TODO: make xxHash optional?
  -DXXH_INLINE_ALL
  )

target_link_libraries(${TARGET} PUBLIC yaml.a
  )

option(OB_USE_BLKID "Use liblkid" ON)
if (${OB_USE_BLKID})
  target_compile_definitions(${TARGET}
    PRIVATE
    -DOB_USE_BLKID
    )
  target_link_libraries(${TARGET} PUBLIC blkid uuid)
endif()
