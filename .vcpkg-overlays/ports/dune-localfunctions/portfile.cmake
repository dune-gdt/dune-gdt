set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(OUT_SOURCE_PATH SOURCE_PATH URL "https://github.com/dune-mirrors/dune-localfunctions.git" REF
               149c7866a11cab6ae5602b50b9f815088daa7a49)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dune-localfunctions)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

if(EXISTS "${SOURCE_PATH}/COPYING")
  file(
    INSTALL "${SOURCE_PATH}/COPYING"
    DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
    RENAME "copyright")
else()
  file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
       "No license file found in the source repository. Please check the source code for license information.")
endif()
