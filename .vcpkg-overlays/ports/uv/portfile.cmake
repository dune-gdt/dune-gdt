# There is no uv port in upstream vcpkg, and astral-sh does not publish SHA-512 checksums for the prebuilt release
# artifacts (only SHA-256), so this overlay builds uv from source with cargo. We fetch the pinned release tag with
# vcpkg_from_git -- the same hash-free mechanism the dune-* overlay ports use -- rather than vcpkg_from_github, which
# would require a SHA-512 we cannot mint offline. A working Rust toolchain (cargo + rustc, see uv's rust-version in
# Cargo.toml) must be available on PATH for this port to build.

set(VCPKG_BUILD_TYPE release)

vcpkg_from_git(
  OUT_SOURCE_PATH SOURCE_PATH URL "https://github.com/astral-sh/uv.git" REF
  5aa65dd7ad0067a6b702bf490c8c2ffe25c50f39 # 0.11.21
)

find_program(
  CARGO_EXECUTABLE
  NAMES cargo
  PATHS "$ENV{HOME}/.cargo/bin" "$ENV{USERPROFILE}/.cargo/bin")
if(NOT CARGO_EXECUTABLE)
  message(FATAL_ERROR "uv: could not find 'cargo'. Install a Rust toolchain (https://rustup.rs) "
                      "before building this port.")
endif()

# Keep cargo's output out of the (read-only-ish) source tree so AUTO_CLEAN can reclaim it after the tool is copied.
set(CARGO_TARGET_DIR "${CURRENT_BUILDTREES_DIR}/${TARGET_TRIPLET}-rust")

vcpkg_execute_required_process(
  COMMAND
  "${CARGO_EXECUTABLE}"
  build
  --release
  --locked
  --bin
  uv
  --target-dir
  "${CARGO_TARGET_DIR}"
  WORKING_DIRECTORY
  "${SOURCE_PATH}"
  LOGNAME
  "cargo-build-${TARGET_TRIPLET}")

# uv is a standalone CLI: install only the executable as a vcpkg tool.
vcpkg_copy_tools(TOOL_NAMES uv SEARCH_DIR "${CARGO_TARGET_DIR}/release" AUTO_CLEAN)

# It ships no headers or libraries, so the usual include/ tree is absent.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE-APACHE" "${SOURCE_PATH}/LICENSE-MIT")
