# cmake-lint: disable=C0301
vcpkg_from_gitlab(
  GITLAB_URL
  https://gitlab.com
  OUT_SOURCE_PATH
  SOURCE_PATH
  REPO
  alberta-fem/alberta3
  REF
  v3.1.1
  HEAD_REF
  main
  SHA512
  c3912500cf9181be075a5a79f75646cf69539f268e151ff0a33cd1099f379dc2d8b868bd9772d91c43ae9457cb7e658d3cd7b72534b81ccee802a9aac163c3f0
)

# Ideally we'd use the pre-run shell arg in configure_cmake for this, but despite the name the provided script runs
# AFTER the autoconf step
vcpkg_execute_required_process(
  COMMAND
  bash
  -c
  "./generate-alberta-automakefiles.sh"
  WORKING_DIRECTORY
  "${SOURCE_PATH}"
  LOGNAME
  "generate-automakefiles-${TARGET_TRIPLET}")

# alberta doesnt find the libtirpc include directory, so we need to set it manually Configure and build
vcpkg_configure_make(
  SOURCE_PATH
  "${SOURCE_PATH}"
  DETERMINE_BUILD_TRIPLET
  OPTIONS
  --disable-fem-toolbox
  --disable-graphics
  CPPFLAGS=-I${CURRENT_INSTALLED_DIR}/include/tirpc
  CFLAGS=-I${CURRENT_INSTALLED_DIR}/include/tirpc
  LDFLAGS=-L${CURRENT_INSTALLED_DIR}/lib)

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
