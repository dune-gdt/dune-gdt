# cmake-lint: disable=C0301
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_download_distfile(
  ARCHIVE
  URLS
  "https://deb.debian.org/debian/pool/main/libt/libtirpc/libtirpc_1.3.6+ds.orig.tar.gz"
  FILENAME
  "libtirpc-1.3.6+ds.orig.tar.gz"
  SHA512
  1e20007d030df9cde23ae3ab99cabb7977c473f9f08a37154e9f43a15c804826651ada402d2075989df65bfdc55fd7a9bda959120da890df9ccf8111cca5ecc6
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

vcpkg_configure_make(SOURCE_PATH "${SOURCE_PATH}" AUTOCONFIG OPTIONS --disable-gssapi)

vcpkg_install_make()

# Remove unnecessary files
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Handle copyright
file(
  INSTALL "${SOURCE_PATH}/COPYING"
  DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}"
  RENAME copyright)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
