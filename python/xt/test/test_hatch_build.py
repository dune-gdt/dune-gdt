# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2024)
# ~~~

import importlib.util
import pathlib

import pytest

# The hooks subclass the hatchling plugin interfaces, so without hatchling there is nothing to test.
pytest.importorskip("hatchling")

# hatch_build.py lives at the package root (next to pyproject.toml), not inside an importable package, and both the xt
# and gdt copies are named `hatch_build`. Load it from its file path under a unique module name to avoid clashes.
_HATCH_BUILD = pathlib.Path(__file__).resolve().parent.parent / "hatch_build.py"


def _load_hatch_build():
    spec = importlib.util.spec_from_file_location("xt_hatch_build", _HATCH_BUILD)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def _write_version(root, have_mpi):
    version_file = root / "dune" / "xt" / "_version.py"
    version_file.parent.mkdir(parents=True)
    version_file.write_text(
        '__version__ = "1.2.3"\n'
        '__git_revision__ = "1.2.3-0-gdeadbeef"\n'
        f"__have_mpi__ = {have_mpi!r}\n",
        encoding="utf-8",
    )


def test_metadata_hook_without_mpi(tmp_path):
    _write_version(tmp_path, have_mpi=False)
    hook = _load_hatch_build().CustomMetadataHook(str(tmp_path), {})
    metadata = {}
    hook.update(metadata)
    assert metadata["dependencies"] == ["ipython", "numpy", "scipy"]


def test_metadata_hook_with_mpi_adds_mpi4py(tmp_path):
    _write_version(tmp_path, have_mpi=True)
    hook = _load_hatch_build().CustomMetadataHook(str(tmp_path), {})
    metadata = {}
    hook.update(metadata)
    assert metadata["dependencies"] == ["ipython", "numpy", "scipy", "mpi4py"]


def test_build_hook_forces_platform_wheel(tmp_path):
    mod = _load_hatch_build()
    hook = mod.CustomBuildHook(str(tmp_path), {}, None, None, str(tmp_path), "wheel")
    build_data = {}
    hook.initialize("standard", build_data)
    assert build_data["pure_python"] is False
    assert build_data["infer_tag"] is True
