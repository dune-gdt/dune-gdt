# There is no uv port in upstream vcpkg. Rather than build the large uv Rust workspace from source, this overlay
# downloads the official prebuilt release binary from GitHub and verifies it against the SHA-256 published in the
# release's sha256.sum. uv is a standalone CLI, so only the uv/uvx executables are installed, as vcpkg tools under
# tools/uv/.

set(VCPKG_BUILD_TYPE release)

set(UV_VERSION "0.11.21")

# Map the vcpkg triplet to the matching cargo-dist release artifact and its published SHA-256 (see
# https://github.com/astral-sh/uv/releases/download/${UV_VERSION}/sha256.sum).
if(VCPKG_TARGET_IS_WINDOWS)
  if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(UV_ARCHIVE "uv-x86_64-pc-windows-msvc.zip")
    set(UV_SHA256 "ace861f360c6de2babedc1607d0f454b6b09a820dbc8182dc15af927e4df9589")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(UV_ARCHIVE "uv-aarch64-pc-windows-msvc.zip")
    set(UV_SHA256 "74e443f8004022dde57a1bd0d10c097830f9ea8feb4ec927db52cd5d805c2f48")
  endif()
elseif(VCPKG_TARGET_IS_OSX)
  if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(UV_ARCHIVE "uv-x86_64-apple-darwin.tar.gz")
    set(UV_SHA256 "f3c8e5708a84b920c18b691214d54d2b0da6b984789caae95d47c95120cb7765")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(UV_ARCHIVE "uv-aarch64-apple-darwin.tar.gz")
    set(UV_SHA256 "1f921d491ba5ffeea774eb04d6681ecee379101341cbb1500394993b541bf3f4")
  endif()
elseif(VCPKG_TARGET_IS_LINUX)
  if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(UV_ARCHIVE "uv-x86_64-unknown-linux-gnu.tar.gz")
    set(UV_SHA256 "8c88519b0ef0af9801fcdee419bbb12116bd9e6b18e162ae093c932d8b264050")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    set(UV_ARCHIVE "uv-aarch64-unknown-linux-gnu.tar.gz")
    set(UV_SHA256 "88e800834007cc5efd4675f166eb2a51e7e3ad19876d85fa8805a6fb5c922397")
  endif()
endif()

if(NOT DEFINED UV_ARCHIVE)
  message(FATAL_ERROR "uv: no prebuilt release artifact is mapped for triplet '${TARGET_TRIPLET}'.")
endif()

# Download and integrity-check against the official SHA-256. We use file(DOWNLOAD) rather than vcpkg_download_distfile
# (which mandates a SHA-512 that the release does not publish and that we cannot mint offline) so we can validate with
# the upstream-provided SHA-256 directly.
set(UV_ARCHIVE_PATH "${CURRENT_BUILDTREES_DIR}/${UV_ARCHIVE}")
file(
  DOWNLOAD "https://github.com/astral-sh/uv/releases/download/${UV_VERSION}/${UV_ARCHIVE}" "${UV_ARCHIVE_PATH}"
  EXPECTED_HASH SHA256=${UV_SHA256}
  SHOW_PROGRESS)

vcpkg_extract_source_archive(UV_TOOL_DIR ARCHIVE "${UV_ARCHIVE_PATH}" SOURCE_BASE "uv-${UV_VERSION}")

# cargo-dist archives wrap the binaries in a single top-level directory, which vcpkg_extract_source_archive strips,
# leaving uv/uvx directly in UV_TOOL_DIR.
vcpkg_copy_tools(TOOL_NAMES uv uvx SEARCH_DIR "${UV_TOOL_DIR}" AUTO_CLEAN)

# uv ships no headers/libraries, and the release archive carries no license files, so record the dual license by hand.
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
file(WRITE "${CURRENT_PACKAGES_DIR}/share/${PORT}/copyright"
     "uv is distributed under the terms of either the Apache License 2.0 or the MIT " "license, at your option. See "
     "https://github.com/astral-sh/uv/blob/${UV_VERSION}/LICENSE-APACHE and " "LICENSE-MIT for the full texts.\n")
