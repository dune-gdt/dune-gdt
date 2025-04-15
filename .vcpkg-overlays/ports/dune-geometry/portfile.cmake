set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(OUT_SOURCE_PATH SOURCE_PATH URL "${CMAKE_CURRENT_LIST_DIR}/../../../deps/dune-geometry" REF
               5673e95ac364ad3498aed9eaf65e0d224384d15a)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dune-geometry)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(
  INSTALL "${SOURCE_PATH}/LICENSE.md"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
