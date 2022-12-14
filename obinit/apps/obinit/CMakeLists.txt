cmake_minimum_required(VERSION 3.5)

project(obinit-bin LANGUAGES C VERSION 0.1.6)

set(C_STANDARD 11)
set(TARGET obinit-bin)
set(OUTPUT_NAME obinit)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

configure_file(src/Version.h.in Version.h)

option(OB_LOG_STDOUT "Write logs to stdout" ON)
option(OB_LOG_KMSG "Write logs to /dev/kmsg" ON)

add_executable(${TARGET}
  src/main.c
  src/ObArgParser.c
  )
target_link_libraries(${TARGET} obinit)
set_target_properties(${TARGET}
        PROPERTIES OUTPUT_NAME ${OUTPUT_NAME})

if (${OB_LOG_STDOUT})
  target_compile_definitions(${TARGET} PRIVATE -DOB_LOG_STDOUT)
endif()


if (${OB_LOG_KMSG})
  target_compile_definitions(${TARGET} PRIVATE -DOB_LOG_KMSG)
endif()


################# INSTALLATION ###############


install (
  PROGRAMS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUT_NAME}
  DESTINATION /sbin/ COMPONENT bin-obinit
  )

install (
  DIRECTORY
  system/
  DESTINATION / COMPONENT bin-obinit
  )

install (
  PROGRAMS
  system/usr/share/initramfs-tools/scripts/local-bottom/obinit
  DESTINATION /usr/share/initramfs-tools/scripts/local-bottom/ COMPONENT bin-obinit
  )

################## PACKAGING #################

set(INSTALLER_NAME overboot-init)
set(INSTALLER_RELEASE 2)
set(INSTALLER_DESCRIPTION_SUMMARY "Overboot initialization binary")
set(INSTALLER_DEPENDS "initramfs-tools")

include(${OB_CMAKE_DIR}/include/cpack-deb.cmake)

# example build:
# cmake -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain.cmake -DINSTALLER_SYSTEM_ARCH=armhf -DCMAKE_BUILD_TYPE=Release -DRELEASE_TAG=$(git rev-parse --short HEAD) ..
# find . -name CPackConfig.cmake -exec cpack --config {} \;
