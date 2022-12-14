# CPack Debian package template

if(EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
  include(InstallRequiredSystemLibraries)

  set(INSTALLER_SYSTEM_ARCH "amd64" CACHE STRING "Destination system architecture")
  set(INSTALLER_SKIP_DEPENDENCIES OFF CACHE BOOL "Use empty dependency list")

  macro(_SET_IF_NOT VAR VALUE)
    if (NOT ${VAR})
      set(${VAR} ${VALUE})
    endif()
  endmacro()

  macro(_SET_IF_SET VAR_TO_SET VALUE_VAR)
    if (DEFINED ${VALUE_VAR})
      set(${VAR_TO_SET} ${${VALUE_VAR}})
    endif()
  endmacro()

  _set_if_set(PROJECT_VERSION_MAJOR VERSION_MAJOR)
  _set_if_set(PROJECT_VERSION_MINOR VERSION_MINOR)
  _set_if_set(PROJECT_VERSION_PATCH VERSION_PATCH)

  _set_if_not(INSTALLER_NAME ${PROJECT_NAME})
  _set_if_not(INSTALLER_RELEASE 1)

  _set_if_not(INSTALLER_DESCRIPTION_SUMMARY "Overboot debian installer")
  _set_if_not(INSTALLER_DESCRIPTION "")
  _set_if_not(INSTALLER_VENDOR "chodak166")

  _set_if_not(INSTALLER_DEPENDS "") # Installer dependencies, version is optional, eg: "libc6 (>= 2.3.1-6), libc6 (< 2.4)")
  _set_if_not(INSTALLER_REPLACES "")

  if (NOT "${RELEASE_TAG}" STREQUAL "")
    set(INSTALLER_RELEASE "${INSTALLER_RELEASE}+${RELEASE_TAG}")
  endif()

  set(CPACK_DEB_COMPONENT_INSTALL ON)

  set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake")
  set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_CURRENT_BINARY_DIR};${PROJECT_NAME};${CMAKE_INSTALL_DEFAULT_COMPONENT_NAME};/")

  set(VERSION_FULL "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}-${INSTALLER_RELEASE}")

  set(CPACK_GENERATOR "DEB")
  set(CPACK_SET_DESTDIR "on")

  set(CPACK_PACKAGE_DESCRIPTION           "${INSTALLER_DESCRIPTION}")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY   "${INSTALLER_DESCRIPTION_SUMMARY}")
  set(CPACK_PACKAGE_VENDOR                "${INSTALLER_VENDOR}")
  set(CPACK_PACKAGE_CONTACT               "${CPACK_PACKAGE_VENDOR}")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY       "optional")
  set(CPACK_DEBIAN_PACKAGE_SECTION        "misc")
  set(CPACK_PACKAGE_VERSION_MAJOR         ${PROJECT_VERSION_MAJOR})
  set(CPACK_PACKAGE_VERSION_MINOR         ${PROJECT_VERSION_MINOR})
  set(CPACK_PACKAGE_VERSION_PATCH         ${PROJECT_VERSION_PATCH})

  if (NOT ${INSTALLER_SKIP_DEPENDENCIES})
    set(CPACK_DEBIAN_PACKAGE_DEPENDS        "${INSTALLER_DEPENDS}")
  else()
    set(CPACK_DEBIAN_PACKAGE_DEPENDS        "")
  endif()

  set(CPACK_DEBIAN_PACKAGE_REPLACES       "${INSTALLER_REPLACES}")
  set(CPACK_DEBIAN_PACKAGE_RELEASE        ${INSTALLER_RELEASE})
  set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE   ${INSTALLER_SYSTEM_ARCH})
  set(CPACK_OUTPUT_FILE_PREFIX            ${CMAKE_BINARY_DIR}/packages)
  set(CPACK_PACKAGE_NAME                  "${INSTALLER_NAME}")
  set(CPACK_PACKAGE_FILE_NAME             "${INSTALLER_NAME}_${VERSION_FULL}_${INSTALLER_SYSTEM_ARCH}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME      "${CPACK_PACKAGE_FILE_NAME}-src")

  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deb")
    set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/deb/preinst;${CMAKE_CURRENT_SOURCE_DIR}/deb/postinst;${CMAKE_CURRENT_SOURCE_DIR}/deb/prerm;${CMAKE_CURRENT_SOURCE_DIR}/deb/postrm;")
  endif()

  include(CPack)

endif (EXISTS "${CMAKE_ROOT}/Modules/CPack.cmake")
