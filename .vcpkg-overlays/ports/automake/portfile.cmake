# cmake-lint: disable=C0301

# automake is a host build tool whose aclocal/automake scripts drive autoconf at runtime (hence the autoconf host
# dependency). As with autoconf, vcpkg_configure_make bakes the persistent CURRENT_INSTALLED_DIR as the prefix and
# installs the executables under tools/${PORT}/bin, so the baked absolute paths are expected (SKIP_ABSOLUTE_PATHS_CHECK)
# and there are no headers (EMPTY_INCLUDE_FOLDER).
set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)

vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://ftp.gnu.org/gnu/automake/automake-${VERSION}.tar.xz"
  FILENAME
  "automake-${VERSION}.tar.xz"
  SHA512
  8baa16831416a953a743f4e3c0f55cea5ebefe0f5a7a0e390581981d4461d02dc9038415124e974b2ec390c40daaa241802cd7d42c6fafb793f87cf355db2a61
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

vcpkg_configure_make(SOURCE_PATH "${SOURCE_PATH}" DETERMINE_BUILD_TRIPLET)

vcpkg_install_make()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
