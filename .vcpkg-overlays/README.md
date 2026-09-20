# vcpkg overlay triplets

`triplets/` holds the vcpkg triplets this project adds on top of the ones vcpkg
ships. `CMakePresets.json` points `VCPKG_OVERLAY_TRIPLETS` here.

| Triplet | Purpose |
|---------|---------|
| `x64-linux-shared` | `x64-linux` with dynamic library linkage and a release-only dependency build. Selected by the `debug`, `release` and `wheelbuilder-*` presets (and everything inheriting from them): the Python bindings need the dune libraries shared, which in turn requires the vcpkg dependencies to be shared. The file itself documents each deviation (including why `openblas` is pinned static). |

## The ports are not here any more

dune-gdt's DUNE modules and its patched copies of `gmsh`, `mpfr`, `gmp`,
`pybind11`, `lapack-reference`, `uv`, `alberta`, `libtirpc` and the GNU
autotools used to live next to this file as overlay ports. They now live in a
vcpkg git registry:

**<https://github.com/dune-gdt/vcpkg-registry>**

That repo's `README.md` is the reference for how the DUNE ports are generated,
why each hand-maintained port deviates from upstream vcpkg, the current DUNE
2.10 pins, and the upgrade path to 2.11.

`vcpkg-configuration.json` in the repo root wires the registry up: it names the
registry, the commit in it that this project pins (`baseline`), and every
package the registry serves instead of upstream vcpkg. To pick up a port change,
bump that `baseline` to the new commit in the registry.

Overlay *triplets* stay here because a vcpkg registry can only serve ports.

## Changing a port

1. Open a PR against `dune-gdt/vcpkg-registry` (edit `ports/`, then run
   `scripts/update-versions.py` there and commit `ports/` and `versions/`
   together).
2. Once it is merged, bump `baseline` in this repo's `vcpkg-configuration.json`
   to the merge commit.
3. Bump the CI build cache key in `.github/workflows/non_docker_build.yml` if
   the change invalidates prebuilt dependencies.
