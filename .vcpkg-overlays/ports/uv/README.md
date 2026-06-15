# uv

Local overlay port for [uv](https://github.com/astral-sh/uv), astral-sh's Python
package and project manager.

There is no `uv` port in upstream vcpkg, so this one is hand-written. Instead of
building the (large) uv Rust workspace from source, it downloads the official
prebuilt release archive from GitHub and verifies it against the SHA-256
published in the release's
[`sha256.sum`](https://github.com/astral-sh/uv/releases/tag/0.11.21), then
installs the `uv` and `uvx` executables as vcpkg tools under `tools/uv/`.

Prebuilt archives are mapped for `x64`/`arm64` on Linux, macOS and Windows.

To bump the version:

1. Pick the new release tag and read its `sha256.sum`:

   ```bash
   curl -sL https://github.com/astral-sh/uv/releases/download/<version>/sha256.sum
   ```

2. Update `UV_VERSION` and every `UV_SHA256` in `portfile.cmake`, and the
   `version` in `vcpkg.json`.
