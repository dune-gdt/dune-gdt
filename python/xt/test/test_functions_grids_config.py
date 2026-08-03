# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~

import importlib.util
import pathlib

import pytest

# dune/xt/test/functions/grids.py is a codegen *config* module, not part of the dune.xt package: the
# .py configs next to the .tpl suites import it as a plain `grids` module, with their own directory
# on sys.path. Load it by path so the fan-out it drives -- one generated test binary per grid type,
# across ~15 suites under dune/xt/test/functions -- is covered here too.
_grids_py = (
    pathlib.Path(__file__).resolve().parents[3]
    / "dune"
    / "xt"
    / "test"
    / "functions"
    / "grids.py"
)
if not _grids_py.is_file():
    pytest.skip(
        f"codegen config {_grids_py} not available (no source tree)",
        allow_module_level=True,
    )

_spec = importlib.util.spec_from_file_location("grids", _grids_py)
grids = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(grids)


def _names(cache, dims=(1, 2, 3)):
    return [grids.pretty_print(g, d) for g, d in grids.type_and_dim(cache, dims)]


def test_alberta_variants_are_generated_when_the_guard_is_published():
    # Regression test for issue #374: ALBERTA_FOUND was never part of the CMake cache snapshot the
    # codegen reads (find_package(Alberta) sets a normal variable, and under a different casing),
    # so _if_active() always failed closed and both Alberta variants were silently missing from
    # every functions/ suite. dxt_write_codegen_cache() publishes the guard now.
    assert _names({"ALBERTA_FOUND": True}) == [
        "2d_simplex_albertagrid",
        "3d_simplex_albertagrid",
        "1d_cube_yaspgrid",
        "2d_cube_yaspgrid",
        "3d_cube_yaspgrid",
        "1d_cube_onedgrid",
    ]


def test_alberta_variants_are_absent_without_the_guard():
    assert "2d_simplex_albertagrid" not in _names({"ALBERTA_FOUND": False})
    # a guard missing from the cache entirely must fail closed, not raise
    assert "2d_simplex_albertagrid" not in _names({})


def test_all_grid_managers_enabled():
    cache = {"ALBERTA_FOUND": True, "dune-alugrid": True, "dune-uggrid": True}
    names = _names(cache, dims=(2,))
    assert names == [
        "2d_simplex_albertagrid",
        "2d_cube_alunonconformgrid",
        "2d_simplex_alunonconformgrid",
        "2d_simplex_aluconformgrid",
        "2d_cube_yaspgrid",
        "2d_simplex_uggrid",
    ]


def test_pretty_print_rejects_unknown_grid():
    with pytest.raises(RuntimeError):
        grids.pretty_print("Dune::SomeOtherGrid<2,2>", 2)
