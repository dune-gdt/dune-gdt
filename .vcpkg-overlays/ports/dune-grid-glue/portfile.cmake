set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(OUT_SOURCE_PATH SOURCE_PATH URL "${CMAKE_CURRENT_LIST_DIR}/../../../deps/dune-grid-glue" REF
               ba82e60a4c7b334668697370c769409dbfbc2bba)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dune-grid-glue)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(
  INSTALL "${SOURCE_PATH}/COPYING"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
