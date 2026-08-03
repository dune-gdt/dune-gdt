# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# ~~~

"""Regenerate the committed uv.lock files for python/xt and python/gdt without a CMake build directory.

Both packages read their version (and, via hatch_build.py, their build-time-dynamic dependencies) from a
CMake-generated dune/{xt,gdt}/_version.py. That file exists only in a binary/assembly directory, so a clean source
checkout cannot build the project metadata that `uv lock` needs -- `uv lock` would fail in `[tool.hatch.version]` with
"file does not exist: dune/xt/_version.py".

This helper drops a throwaway placeholder _version.py into each source package (mirroring the shape CMake configures
from _version.py.in), runs `uv lock`, and removes the placeholder again. The placeholder version is irrelevant to the
resulting lockfile: uv records the root project as an editable source without a version field, and the dune.gdt ->
dune.xt edge is resolved through [tool.uv.sources] as the local ../xt path, so neither the pinned version nor the exact
placeholder value is written into the lock. This is why the committed lockfiles stay valid at test time even though the
real build stamps a different, git-describe-based version into _version.py.

Note: the placeholder sets __have_mpi__ = False, so the base runtime dependency list locked here does not include the
mpi4py that hatch_build.py adds for MPI-enabled builds (the CI presets set CMAKE_DISABLE_FIND_PACKAGE_MPI=TRUE, so the
tested builds are non-MPI). An MPI-enabled build that wants a lock covering mpi4py has to regenerate from its binary dir.

Usage:

    python python/update_lockfiles.py          # regenerate both lockfiles
    uv run --no-project python python/update_lockfiles.py
"""

import subprocess
import sys
from contextlib import contextmanager
from pathlib import Path

HERE = Path(__file__).resolve().parent

# The CMake-generated _version.py assigns __version__/__git_revision__ (both packages) and __have_mpi__ (xt only); we
# emit a superset with a neutral placeholder version. Using the same version in both packages keeps the dune.gdt ->
# dune.xt exact-version pin (dune.xt==<version>, see python/gdt/hatch_build.py) internally consistent while locking.
_PLACEHOLDER_VERSION = "0.0.0"
_PLACEHOLDER = (
    f'__version__ = "{_PLACEHOLDER_VERSION}"\n'
    f'__git_revision__ = "{_PLACEHOLDER_VERSION}"\n'
    "__have_mpi__ = False\n"
)

# (project directory relative to this file, path of the generated _version.py within it)
PROJECTS = [
    ("xt", Path("dune") / "xt" / "_version.py"),
    ("gdt", Path("dune") / "gdt" / "_version.py"),
]


@contextmanager
def placeholder_version(version_file: Path):
    """Write a throwaway _version.py so hatchling can compute metadata, and remove it again afterwards.

    Refuses to overwrite an existing _version.py (e.g. when accidentally run inside a configured binary dir), so a real
    build's file is never clobbered.
    """
    if version_file.exists():
        raise SystemExit(
            f"refusing to overwrite existing {version_file}; run this from a clean source checkout, not a build dir"
        )
    version_file.parent.mkdir(parents=True, exist_ok=True)
    version_file.write_text(_PLACEHOLDER, encoding="utf-8")
    try:
        yield
    finally:
        version_file.unlink(missing_ok=True)


def main() -> int:
    for project, version_rel in PROJECTS:
        project_dir = HERE / project
        with placeholder_version(project_dir / version_rel):
            print(f"-- locking python/{project} ...")
            subprocess.run(["uv", "lock"], cwd=project_dir, check=True)
    print("-- done; review and commit python/xt/uv.lock and python/gdt/uv.lock")
    return 0


if __name__ == "__main__":
    sys.exit(main())
