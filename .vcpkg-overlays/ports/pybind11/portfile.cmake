set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL "${CMAKE_CURRENT_LIST_DIR}/../../../deps/pybind11"
    REF a2e59f0e7065404b44dfe92a28aca47ba1378dc4
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DBUILD_TESTING=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/pybind11)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(
    INSTALL "${SOURCE_PATH}/LICENSE"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME copyright
)
