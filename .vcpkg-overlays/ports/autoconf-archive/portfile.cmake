# cmake-lint: disable=C0301

# autoconf-archive is data-only: it installs a large collection of reusable m4 macros (consumed by aclocal/autoconf at
# configure time) under share/${PORT}/aclocal and ships no binaries or headers, hence EMPTY_INCLUDE_FOLDER.
set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://ftp.gnu.org/gnu/autoconf-archive/autoconf-archive-${VERSION}.tar.xz"
  FILENAME
  "autoconf-archive-${VERSION}.tar.xz"
  SHA512
  91140cb666447f12a1d39d7d42f5fe6ea3601bb586f466381c9e949087aafa06aed8d061a4f5d020a3d234eb710e4bb47cd25380bcffd8c423fa1a7af05e988b
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

vcpkg_configure_make(SOURCE_PATH "${SOURCE_PATH}" DETERMINE_BUILD_TRIPLET)

vcpkg_install_make()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/COPYING")
