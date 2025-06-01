set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CHAINLOAD_TOOLCHAIN_FILE ${CMAKE_CURRENT_LIST_DIR}/../toolchains/linux-clang.cmake)

# make changes in this file add to the ABI hash
set(VCPKG_HASH_ADDITIONAL_FILES ${CMAKE_CURRENT_LIST_DIR}/../toolchains/clang_include.cmake
                                ${CMAKE_CURRENT_LIST_DIR}/../toolchains/try_compile_common.cmake)
