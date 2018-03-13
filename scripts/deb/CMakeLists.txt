#Original source of the deb packager graciously created by https://bitbucket.org/tasptz/cmake_adawebide

cmake_minimum_required(VERSION 2.8)
project(Adafruit)

set(CPACK_GENERATOR "DEB")

set(EDITOR_DIR "${CMAKE_SOURCE_DIR}/webide")
set(WEBIDE_ROOT "/usr/share/adafruit/webide")
set(WEBIDE_HOME "/home/webide")
set(P755 OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
set(P440 OWNER_READ GROUP_READ)

install(FILES "${EDITOR_DIR}/scripts/adafruit-webide.sh"
        DESTINATION "/etc/init.d"
        PERMISSIONS ${P755})
install(FILES "${EDITOR_DIR}/scripts/.bashrc" DESTINATION ${WEBIDE_HOME})
install(FILES "adafruit" DESTINATION "/etc/sudoers.d/" PERMISSIONS ${P440})
install(DIRECTORY "${EDITOR_DIR}/" DESTINATION ${WEBIDE_ROOT})

set(CPACK_PACKAGE_NAME "adafruitwebide")
set(CPACK_PACKAGE_CONTACT "support@adafruit.com")
set(CPACK_PACKAGE_VENDOR "Adafruit")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "3")
set(CPACK_PACKAGE_VERSION_PATCH "12")
set(CPACK_PACKAGE_VERSION "0.3.12")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Adafruit WebIDE")
set(CPACK_RESOURCE_FILE_README "${EDITOR_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${EDITOR_DIR}/GNU-AGPL-3.0")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Adafruit")
set(CPACK_DEBIAN_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION}-ubuntu")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SECTION "misc")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "nodejs, nodejs-legacy, redis-server, git, avahi-daemon, i2c-tools, python-smbus")
set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/postinst;${CMAKE_CURRENT_SOURCE_DIR}/prerm;${CMAKE_CURRENT_SOURCE_DIR}/postrm;")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")

include(CPack)
