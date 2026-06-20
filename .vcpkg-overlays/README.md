# vcpkg overlay ports for DUNE modules

dune-gdt builds its DUNE dependencies from source through local vcpkg overlay
ports in `ports/`. This document explains how those ports are produced, the pin
policy in effect, and the work required to move from DUNE 2.10 to a newer
release.

## How the ports are generated

The ports are **generated, not hand-edited**. The canonical source of truth is
`deps/module_list.bash`, which maps each DUNE module to a git URL and a commit
hash:

```bash
SUBMODULE_INFO_HASH['dune-istl']='...'
SUBMODULE_INFO_URL['dune-istl']='https://github.com/dune-mirrors/dune-istl.git'
```

`update_ports.bash` reads that file and (re)writes
`ports/<module>/vcpkg.json` + `ports/<module>/portfile.cmake` for every module.
The port `version` is taken from each module's `dune.module` `Version:` field at
the pinned commit. To refresh a pin, edit `deps/module_list.bash` and rerun:

```bash
.vcpkg-overlays/update_ports.bash
```

> Note: `update_ports.bash` clones every module, including the GitLab-hosted
> `dune-alugrid` and `dune-uggrid`. Networks that block `gitlab.dune-project.org`
> (e.g. the Claude Code web sandbox) cannot run the full regeneration; in that
> case edit `deps/module_list.bash` and the affected `portfile.cmake` together by
> hand, keeping them in sync.

## Hand-maintained ports (not generated)

A handful of ports in `ports/` are **not** produced by `update_ports.bash` and
are edited by hand: `pybind11`, `uv`, `libtirpc`, `alberta`, and the GNU
autotools host tools below. `update_ports.bash` only touches the DUNE modules
listed in `deps/module_list.bash`, so it leaves these alone.

### GNU autotools host tools

`libtirpc` and `alberta` build with `vcpkg_configure_make`, which needs the GNU
autotools. To avoid depending on whatever happens to be installed on the build
machine, the following ports pin current upstream releases (downloaded from
`ftp.gnu.org` and verified by SHA-512):

| Port | Version | Notes |
|------|---------|-------|
| autoconf | 2.73 | `autoconf`, `autoreconf`, `autom4te`, ... under `tools/autoconf/bin` |
| automake | 1.18.1 | `automake`/`aclocal`; depends on `autoconf` (host) |
| libtool | 2.5.4 | `libtool`/`libtoolize` plus the `libltdl` loader library + `ltdl.h` |
| autoconf-archive | 2024.10.16 | data-only: ~580 reusable m4 macros under `share/autoconf-archive/aclocal` |

`vcpkg_configure_make` bakes the persistent `CURRENT_INSTALLED_DIR` into the
installed scripts as their prefix and installs the executables under
`tools/<port>/bin`, so the tools remain usable after the temporary build trees
are gone. The aclocal macros for each port live in `share/<port>/aclocal`; a
consumer that wires these tools onto `PATH` should also add those directories to
`ACLOCAL_PATH` (for example `share/libtool/aclocal` and
`share/autoconf-archive/aclocal`).

To bump a version: update `version` in the port's `vcpkg.json`, the URL/SHA-512
in its `portfile.cmake` (`sha512sum` of the new `.tar.xz`), and rebuild.

> Note: the bare `libtool` entry in the repo's top-level `.gitignore` (an
> autotools-generated script name) also matches `ports/libtool/`, so that path is
> re-included there with a `!` negation. Keep that negation when editing
> `.gitignore`.

## Current pins (DUNE 2.10)

All core/staging modules currently track the **`releases/2.10` maintenance
branch** (i.e. 2.10.x plus accumulated bugfixes), not the exact `v2.10.0` tags.

| Port | Source repo | Notes |
|------|-------------|-------|
| dune-common | `dune-community/dune-common` | custom fork branch `releases/2.10_superbuild_hack` (carries downstream patches) |
| dune-geometry | `dune-mirrors/dune-geometry` | `releases/2.10` HEAD |
| dune-grid | `dune-mirrors/dune-grid` | `releases/2.10` HEAD |
| dune-istl | `dune-mirrors/dune-istl` | `releases/2.10` HEAD |
| dune-localfunctions | `dune-mirrors/dune-localfunctions` | `releases/2.10` HEAD |
| dune-grid-glue | `dune-mirrors/dune-grid-glue` | `releases/2.10` HEAD |
| dune-alugrid | `gitlab .../extensions/dune-alugrid` | 2.10 |
| dune-uggrid | `gitlab .../staging/dune-uggrid` | 2.10 |
| dune-testtools | `dune-community/dune-testtools` | community fork |

The core modules are sourced from GitHub mirrors (`dune-mirrors/*`,
`dune-community/*`) rather than upstream `gitlab.dune-project.org` so that builds
work on networks where GitLab is not reachable.

### Pin policy caveat

Pins currently point at *moving branch HEADs* rather than immutable tags, so the
same `portfile.cmake` can resolve to different sources as the mirrors advance.
For reproducible builds, prefer pinning to the exact release tag commit
(`v2.10.x`) and bumping deliberately.

## Upgrade path to DUNE 2.11

DUNE 2.11.0 was released 2026-02-07 (2.10.0 was 2024-10-23). Moving the ports to
2.11 is gated on more than editing hashes:

1. **The GitHub mirrors are stale.** `dune-mirrors/*` and `dune-community/*` have
   no `releases/2.11` branch and no `v2.11.0` tag — only `releases/2.10` and
   `master`. There is nothing 2.11-shaped to pin to on GitHub yet. Either push
   `releases/2.11` to the mirrors, or allowlist `gitlab.dune-project.org` and
   point the URLs upstream.
2. **dune-common carries downstream patches.** It is pinned to the custom fork
   branch `releases/2.10_superbuild_hack`. A `releases/2.11_superbuild_hack`
   equivalent must be rebased on the fork first — this is the gating work item.
3. **API churn.** The biggest porting risk in 2.10→2.11 is the
   typetree → dune-common migration (`dune-functions`'s dependency on
   `dune-typetree` is being downgraded/removed, completing in 2.12) and the
   expanded C++20 concepts in dune-common/dune-grid. dune-gdt's grid-view and
   local-function code is the likely friction point.

Once mirrored and patched, the mechanical steps are: update the hashes/URLs in
`deps/module_list.bash`, rerun `update_ports.bash`, build-test, bump the CI build
cache key in `.github/workflows/non_docker_build.yml`, and (optionally) raise the
`Depends`/`Suggests` floor in `dune.module` from `>= 2.8`.
