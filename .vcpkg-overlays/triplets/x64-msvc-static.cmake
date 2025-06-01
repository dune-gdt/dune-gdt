# upstream triplet plus custom toolchain
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE
    dynamic
    CACHE STRING "Linkage of the C runtime library" FORCE)
set(VCPKG_LIBRARY_LINKAGE
    static
    CACHE STRING "Linkage of compiled libraries" FORCE)

# make changes in this file add to the ABI hash
set(VCPKG_HASH_ADDITIONAL_FILES ${CMAKE_CURRENT_LIST_DIR}/../toolchains/msvc_include.cmake
                                ${CMAKE_CURRENT_LIST_DIR}/../../cmake/toolchains/msvc/Windows_MSVC_toolchain.cmake)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../toolchains/win-msvc.cmake)
# on Windows VCPKG_CHAINLOAD_TOOLCHAIN_FILE deactivates VS variables, so those need to be loaded again
set(VCPKG_LOAD_VCVARS_ENV ON)
