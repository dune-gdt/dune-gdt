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
"""WP0 (#320): the binding-discovery helpers behind the shared hypothesis strategies.

`dune.xt.test.hypothesis_strategies` no longer hard-codes which grid/container combinations the
wheel was built with; it probes the compiled `dune.xt.grid` / `dune.xt.la` modules instead. The
parsing and filtering that drives that probe is pure Python and is unit-tested here against stub
modules, so it runs without the compiled bindings. A second group of tests exercises the real
bindings when they are importable and asserts the discovered slice is self-consistent.
"""

import types

import pytest

from dune.xt.test import hypothesis_strategies as hs


def _stub_module(class_names):
    """A stand-in for a compiled binding module exposing the given names as classes."""
    module = types.SimpleNamespace()
    for name in class_names:
        setattr(module, name, type(name, (), {}))
    return module


# The full-build GridProvider* class names and the (dim, element) slice they encode.
_FULL_BUILD_GRID_CLASSES = {
    "GridProvider1dSimplexOnedgrid": (1, "simplex", "onedgrid"),
    "GridProvider2dCubeYaspgrid": (2, "cube", "yaspgrid"),
    "GridProvider2dSimplexAluconformgrid": (2, "simplex", "aluconformgrid"),
    "GridProvider3dCubeYaspgrid": (3, "cube", "yaspgrid"),
    "GridProvider3dSimplexAluconformgrid": (3, "simplex", "aluconformgrid"),
}


@pytest.mark.parametrize(("name", "expected"), _FULL_BUILD_GRID_CLASSES.items())
def test_parse_grid_provider_name_extracts_coordinates(name, expected):
    assert hs._parse_grid_provider_name(name) == expected


@pytest.mark.parametrize(
    "name",
    [
        "GridProviderFactory",  # a factory, not a provider instantiation
        "CouplingGridProvider2dCubeYaspgrid",  # does not start with GridProvider
        "CommonVector",  # unrelated class
        "GridProvider",  # no dimension/element
        "GridProvider2dCube",  # no implementation word
    ],
)
def test_parse_grid_provider_name_rejects_non_providers(name):
    assert hs._parse_grid_provider_name(name) is None


def test_grid_binding_reconstructs_the_grid_name():
    binding = hs.GridBinding(
        dim=2, element="cube", impl="yaspgrid", class_name="GridProvider2dCubeYaspgrid"
    )
    assert binding.grid_name == "2d_cube_yaspgrid"


def test_discover_grid_bindings_parses_and_sorts_a_full_build():
    module = _stub_module(list(_FULL_BUILD_GRID_CLASSES) + ["GridProviderFactory"])
    bindings = hs.discover_grid_bindings(module)
    assert [b.grid_name for b in bindings] == [
        "1d_simplex_onedgrid",
        "2d_cube_yaspgrid",
        "2d_simplex_aluconformgrid",
        "3d_cube_yaspgrid",
        "3d_simplex_aluconformgrid",
    ]


def test_discover_grid_combinations_matches_the_previously_hard_coded_tuple():
    # This is exactly the tuple WP0 replaced; discovery must reproduce it on a full build.
    module = _stub_module(_FULL_BUILD_GRID_CLASSES)
    assert hs.discover_grid_combinations(module) == (
        (1, "simplex"),
        (2, "cube"),
        (2, "simplex"),
        (3, "cube"),
        (3, "simplex"),
    )


def test_discover_grid_combinations_deduplicates_and_sorts():
    # Two cube implementations for the same dimension collapse to a single (dim, element).
    module = _stub_module(
        ["GridProvider2dCubeYaspgrid", "GridProvider2dCubeAlunonconformgrid"]
    )
    assert hs.discover_grid_combinations(module) == ((2, "cube"),)


def test_discovery_picks_up_a_newly_bound_combination_without_edits():
    # A later work package binding, e.g., a 2d cube ALUGrid must appear automatically.
    baseline = _stub_module(_FULL_BUILD_GRID_CLASSES)
    extended = _stub_module(
        list(_FULL_BUILD_GRID_CLASSES) + ["GridProvider2dCubeAluconformgrid"]
    )
    new = set(hs.discover_grid_combinations(extended)) - set(
        hs.discover_grid_combinations(baseline)
    )
    # (2, "cube") already exists via YASP, so the *combination* set is unchanged...
    assert new == set()
    # ...but the richer binding list does gain the new implementation.
    impls = {b.class_name for b in hs.discover_grid_bindings(extended)}
    assert "GridProvider2dCubeAluconformgrid" in impls


def test_import_optional_returns_none_for_a_missing_module():
    # An absent optional dependency must degrade to None (not raise), so a build without, e.g.,
    # Eigen still imports the strategies module and its dependent tests simply skip.
    assert hs._import_optional("dune.xt._definitely_not_a_real_module") is None


def test_grid_combinations_is_always_a_tuple():
    # Computed at import time from whatever the build exposes (possibly empty); importing the
    # strategies module must never fail, whether or not the compiled grids are present.
    assert isinstance(hs.GRID_COMBINATIONS, tuple)


def test_discover_vector_types_excludes_the_index_container():
    module = _stub_module(
        [
            "CommonVector",
            "IstlVector",
            "EigenVector",
            "CommonVectorSizeT",  # size_t index vector: not a float payload container
            "CommonDenseMatrix",
        ]
    )
    names = [c.__name__ for c in hs.discover_vector_types(module)]
    assert names == ["CommonVector", "EigenVector", "IstlVector"]


def test_discover_matrix_types_excludes_the_sparsity_pattern():
    module = _stub_module(
        [
            "CommonDenseMatrix",
            "IstlSparseMatrix",
            "EigenSparseMatrix",
            "SparsityPatternDefault",  # not a container
            "CommonVector",
        ]
    )
    names = [c.__name__ for c in hs.discover_matrix_types(module)]
    assert names == ["CommonDenseMatrix", "EigenSparseMatrix", "IstlSparseMatrix"]


def test_discovery_ignores_non_class_attributes():
    def make_common_vector():  # a factory function, not a container class
        return None

    module = types.SimpleNamespace()
    module.CommonVector = type("CommonVector", (), {})
    module.make_common_vector = make_common_vector
    assert [c.__name__ for c in hs.discover_vector_types(module)] == ["CommonVector"]


# --- against the real bindings, when this build actually provides them --------------------


@pytest.fixture(scope="module")
def grid_module():
    return pytest.importorskip("dune.xt.grid")


@pytest.fixture(scope="module")
def la_module():
    return pytest.importorskip("dune.xt.la")


def test_real_grid_combinations_are_nonempty_and_well_formed(grid_module):
    combinations = hs.discover_grid_combinations(grid_module)
    assert combinations, (
        "a build with dune.xt.grid must expose at least one grid provider"
    )
    assert combinations == hs.GRID_COMBINATIONS
    for dim, element in combinations:
        assert isinstance(dim, int) and dim >= 1
        assert element in ("cube", "simplex", "prism", "mixed")


def test_has_grid_is_consistent_with_the_discovered_combinations(grid_module):
    for dim, element in hs.GRID_COMBINATIONS:
        assert hs.has_grid(dim=dim, element=element)
        assert hs.has_grid(dim=dim)
        assert hs.has_grid(element=element)
    assert not hs.has_grid(dim=99)


def test_real_vector_and_matrix_types_are_exposed_classes(la_module):
    vectors = hs.discover_vector_types(la_module)
    assert vectors, "a build with dune.xt.la must expose at least one vector container"
    for cls in vectors:
        assert isinstance(cls, type)
        assert getattr(la_module, cls.__name__) is cls
        assert not cls.__name__.endswith("SizeT")
    for cls in hs.discover_matrix_types(la_module):
        assert isinstance(cls, type)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
