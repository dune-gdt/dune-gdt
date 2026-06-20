# cmake-lint: disable=C0301

# autoconf is a host build tool (perl/m4 scripts plus a tree of m4 macros). vcpkg_configure_make bakes the persistent
# CURRENT_INSTALLED_DIR into the installed scripts as their prefix and places the executables under tools/${PORT}/bin,
# so the tool keeps working after the temporary build/package trees are gone. The baked absolute paths are expected,
# hence SKIP_ABSOLUTE_PATHS_CHECK; autoconf ships no headers, hence EMPTY_INCLUDE_FOLDER.
set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_SKIP_ABSOLUTE_PATHS_CHECK enabled)

vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://ftp.gnu.org/gnu/autoconf/autoconf-${VERSION}.tar.xz"
  FILENAME
  "autoconf-${VERSION}.tar.xz"
  SHA512
  be051d542f9bd93752eccbdc9b1f3cf1fa4069a153ef34edd5303bfdf162e24d56e33e70df1b978cebb8d1139a82417bb8299cfe6c38cff8613040a829cc624f
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

vcpkg_configure_make(SOURCE_PATH "${SOURCE_PATH}" DETERMINE_BUILD_TRIPLET)

vcpkg_install_make()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
