# cmake-lint: disable=C0301

# autoconf-archive is data-only: it installs a large collection of reusable m4 macros (consumed by aclocal/autoconf at
# configure time) under share/${PORT}/aclocal and ships no binaries or headers, hence EMPTY_INCLUDE_FOLDER.
set(VCPKG_BUILD_TYPE release)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

# Fetch from several GNU mirrors, not just ftp.gnu.org: that host repeatedly timed out in CI while building this port,
# and since every build leg (the C++ matrix and the wheel build) rebuilds the vcpkg dependencies from scratch, one slow
# mirror blocks the whole pipeline. vcpkg_download_distfile tries the URLs in order and verifies the SHA512 of whichever
# responds, so the extra mirrors are pure resiliency -- mirrors.kernel.org and the ftpmirror.gnu.org redirector are
# listed ahead of ftp.gnu.org as the more reliable primaries.
vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://mirrors.kernel.org/gnu/autoconf-archive/autoconf-archive-${VERSION}.tar.xz"
  "https://ftpmirror.gnu.org/autoconf-archive/autoconf-archive-${VERSION}.tar.xz"
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
