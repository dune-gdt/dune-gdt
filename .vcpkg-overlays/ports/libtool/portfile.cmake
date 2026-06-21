# cmake-lint: disable=C0301

# libtool provides the libtool/libtoolize host scripts (used together with autoconf/automake, hence the host
# dependencies) plus the libltdl dynamic-module-loader library and its ltdl.h header. vcpkg_configure_make bakes the
# persistent CURRENT_INSTALLED_DIR as the prefix and installs the scripts under tools/${PORT}/bin, so the baked absolute
# paths in those scripts are expected (SKIP_ABSOLUTE_PATHS_CHECK).
set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)

vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://ftp.gnu.org/gnu/libtool/libtool-${VERSION}.tar.xz"
  FILENAME
  "libtool-${VERSION}.tar.xz"
  SHA512
  eed207094bcc444f4bfbb13710e395e062e3f1d312ca8b186ab0cbd22dc92ddef176a0b3ecd43e02676e37bd9e328791c59a38ef15846d4eae15da4f20315724
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

# --enable-ltdl-install installs the libltdl loader library and ltdl.h alongside the libtool scripts.
vcpkg_configure_make(SOURCE_PATH "${SOURCE_PATH}" DETERMINE_BUILD_TRIPLET OPTIONS --enable-ltdl-install)

vcpkg_install_make()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_fixup_pkgconfig()

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
