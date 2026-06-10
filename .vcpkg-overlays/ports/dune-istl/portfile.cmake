set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(OUT_SOURCE_PATH SOURCE_PATH URL "https://github.com/dune-mirrors/dune-istl.git" REF
               21c67275b17e93918365177f93f42e4aaa9afd23)

vcpkg_cmake_configure(SOURCE_PATH "${SOURCE_PATH}" OPTIONS -DBUILD_TESTING=OFF -DCMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/dune-istl)

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
