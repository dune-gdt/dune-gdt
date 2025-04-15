set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(OUT_SOURCE_PATH SOURCE_PATH URL "${CMAKE_CURRENT_LIST_DIR}/../../../deps/dune-grid" REF
               954436b88247e904628ec4d7c8bb7b2eaac08900)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dune-grid)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(
  INSTALL "${SOURCE_PATH}/LICENSE.md"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)
