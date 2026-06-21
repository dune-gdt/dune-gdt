vcpkg_minimum_required(VERSION 2022-10-12) # for ${VERSION}

vcpkg_download_distfile(ARCHIVE
    URLS "http://www.mpfr.org/mpfr-${VERSION}/mpfr-${VERSION}.tar.xz" "https://ftp.gnu.org/gnu/mpfr/mpfr-${VERSION}.tar.xz"
    FILENAME "mpfr-${VERSION}.tar.xz"
    SHA512 eb9e7f51b5385fb349cc4fba3a45ffdf0dd53be6dfc74932dc01258158a10514667960c530c47dd9dfc5aa18be2bd94859d80499844c5713710581e6ac6259a9
)

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
    PATCHES
        dll.patch
        src-only.patch
)

# mpfr is reconfigured with autoreconf, which needs autoconf/automake/libtool and
# the autoconf-archive m4 macros (its configure.ac uses AX_* macros). Those come
# from our overlay host-tool ports (declared as host dependencies in vcpkg.json),
# but vcpkg does not put a host dependency's tools/<port>/bin on PATH automatically
# and aclocal does not search the per-port share/<port>/aclocal macro dirs. Wire
# both up here so the build does not fall back to the system autotools. See
# .vcpkg-overlays/README.md.
vcpkg_add_to_path(PREPEND "${CURRENT_HOST_INSTALLED_DIR}/tools/autoconf/bin")
vcpkg_add_to_path(PREPEND "${CURRENT_HOST_INSTALLED_DIR}/tools/automake/bin")
vcpkg_add_to_path(PREPEND "${CURRENT_HOST_INSTALLED_DIR}/tools/libtool/bin")
file(GLOB mpfr_aclocal_dirs LIST_DIRECTORIES true
    "${CURRENT_HOST_INSTALLED_DIR}/share/aclocal"
    "${CURRENT_HOST_INSTALLED_DIR}/share/*/aclocal"
)
# aclocal splits ACLOCAL_PATH on ';' on native Windows hosts and ':' elsewhere.
if(CMAKE_HOST_WIN32)
    set(mpfr_path_sep ";")
else()
    set(mpfr_path_sep ":")
endif()
list(JOIN mpfr_aclocal_dirs "${mpfr_path_sep}" mpfr_aclocal_path)
set(ENV{ACLOCAL_PATH} "${mpfr_aclocal_path}")

vcpkg_make_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    AUTORECONF
)

vcpkg_make_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(REMOVE
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/AUTHORS"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/BUGS"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/COPYING"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/COPYING.LESSER"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/NEWS"
    "${CURRENT_PACKAGES_DIR}/share/${PORT}/TODO"
)

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING" "${SOURCE_PATH}/COPYING.LESSER")
