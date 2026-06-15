# uv

Local overlay port for [uv](https://github.com/astral-sh/uv), astral-sh's Python
package and project manager.

There is no `uv` port in upstream vcpkg, so this one is hand-written. It builds
uv from source with `cargo` (the source is fetched hash-free via
`vcpkg_from_git`, matching the dune-* overlay ports) and installs the resulting
`uv` executable as a vcpkg tool under `tools/uv/`.

Building requires a Rust toolchain (`cargo` + `rustc`) on `PATH`; see uv's
`rust-version` in its `Cargo.toml` for the minimum supported Rust.

To bump the version, change the `version` in `vcpkg.json` and the `REF` (and its
trailing version comment) in `portfile.cmake` to the matching release tag's
commit hash:

```bash
git ls-remote --tags https://github.com/astral-sh/uv.git 'refs/tags/<version>'
```
