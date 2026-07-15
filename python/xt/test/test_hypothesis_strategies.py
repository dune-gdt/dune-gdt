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


# --- impl-aware GridSpec (WP1): conforming classification is pure Python ------------------


@pytest.mark.parametrize(
    ("impl", "conforming"),
    [
        (
            None,
            True,
        ),  # the make_cube_grid default (structured cube / conforming simplex)
        ("yaspgrid", True),
        ("onedgrid", True),
        ("aluconformgrid", True),
        (
            "alunonconformgrid",
            False,
        ),  # both the ALU cube and the nonconforming ALU simplex grid
    ],
)
def test_grid_spec_conforming_reflects_the_implementation(impl, conforming):
    spec = hs.GridSpec(
        dim=2,
        element="cube",
        lower_left=(0.0, 0.0),
        upper_right=(1.0, 1.0),
        num_elements=(1, 1),
        impl=impl,
    )
    assert spec.conforming is conforming
    assert hs._impl_is_conforming(impl) is conforming


def test_grid_spec_defaults_to_the_conforming_default_implementation():
    # A GridSpec built without an impl (as the interpolation-points subprocess does) must keep the
    # pre-#320 behaviour: the structured/conforming make_cube_grid default, hence conforming.
    spec = hs.GridSpec(
        dim=1,
        element="simplex",
        lower_left=(0.0,),
        upper_right=(1.0,),
        num_elements=(2,),
    )
    assert spec.impl is None
    assert spec.conforming is True


def test_sampled_grid_bindings_filters_dims_elements_and_conformity(monkeypatch):
    # A stub mirroring the bindable ALUGrid build (#320 WP1): the 3d cube slice exposes both the
    # structured YASP grid and the nonconforming ALU hexahedral grid, while every simplex slice has
    # only the conforming ALU grid (conforming and nonconforming ALU simplex grids share a leaf-view
    # type and cannot both be bound; there is no 2d cube ALUGrid) -- see grids.bindings.hh.
    stub = [
        hs.GridBinding(1, "simplex", "onedgrid", "GridProvider1dSimplexOnedgrid"),
        hs.GridBinding(2, "cube", "yaspgrid", "GridProvider2dCubeYaspgrid"),
        hs.GridBinding(
            2, "simplex", "aluconformgrid", "GridProvider2dSimplexAluconformgrid"
        ),
        hs.GridBinding(3, "cube", "yaspgrid", "GridProvider3dCubeYaspgrid"),
        hs.GridBinding(
            3, "simplex", "aluconformgrid", "GridProvider3dSimplexAluconformgrid"
        ),
        hs.GridBinding(
            3, "cube", "alunonconformgrid", "GridProvider3dCubeAlunonconformgrid"
        ),
    ]
    monkeypatch.setattr(hs, "GRID_BINDINGS", stub)

    # both cube implementations are offered for (3, "cube")
    cubes = hs._sampled_grid_bindings(
        dims=(3,), elements=("cube",), conforming_only=False
    )
    assert set(cubes) == {(3, "cube", "yaspgrid"), (3, "cube", "alunonconformgrid")}

    # conforming_only drops the nonconforming ALU cube grid
    conforming = hs._sampled_grid_bindings(
        dims=(1, 2, 3), elements=("cube", "simplex"), conforming_only=True
    )
    assert all(impl != "alunonconformgrid" for (_, _, impl) in conforming)
    assert (3, "cube", "yaspgrid") in conforming
    assert (3, "simplex", "aluconformgrid") in conforming

    # dimension and element filters compose
    only_cube_3d = hs._sampled_grid_bindings(
        dims=(3,), elements=("cube",), conforming_only=True
    )
    assert set(only_cube_3d) == {(3, "cube", "yaspgrid")}


# --- WP3 (#320): unstructured (UG) mixed-element and prism grid helpers --------------------


def test_has_uggrid_detects_the_ug_provider():
    with_ug = _stub_module([*_FULL_BUILD_GRID_CLASSES, "GridProvider2dSimplexUggrid"])
    without_ug = _stub_module(_FULL_BUILD_GRID_CLASSES)
    assert hs.has_uggrid(with_ug) is True
    assert hs.has_uggrid(without_ug) is False


@pytest.mark.parametrize(
    ("dim", "num_corners", "expected"),
    [
        (1, 2, "cube"),
        (2, 3, "simplex"),
        (2, 4, "cube"),
        (3, 4, "simplex"),
        (3, 5, "pyramid"),
        (3, 6, "prism"),
        (3, 8, "cube"),
        (3, 7, "unknown"),  # no element type has seven corners
    ],
)
def test_classify_element(dim, num_corners, expected):
    assert hs.classify_element(dim, num_corners) == expected


@pytest.mark.parametrize(
    ("geometry_type", "order", "dim", "expected"),
    [
        ("cube", 0, 2, 1),
        ("cube", 1, 2, 4),  # Q_1 on a quad
        ("cube", 2, 3, 27),  # Q_2 on a hex
        ("simplex", 1, 2, 3),  # P_1 on a triangle
        ("simplex", 2, 3, 10),  # P_2 on a tet
        ("prism", 1, 3, 6),  # the six vertices of a prism
    ],
)
def test_dg_dofs_per_element(geometry_type, order, dim, expected):
    assert hs.dg_dofs_per_element(geometry_type, order, dim) == expected


def test_dg_dofs_per_element_rejects_unknown_geometry():
    with pytest.raises(NotImplementedError):
        hs.dg_dofs_per_element("unknown", 1, 2)


class _FakeMixedGrid:
    """A stand-in GridProvider: two quads and two triangles in 2d, walked corner-count first."""

    dimension = 2
    _corner_counts = (4, 4, 3, 3)

    def apply_on_each_element(self, visitor):
        for corners in self._corner_counts:
            visitor(types.SimpleNamespace(corners=_Shape((corners, 2))))


class _Shape:
    def __init__(self, shape):
        self.shape = shape


def test_element_geometry_counts_on_a_mixed_grid():
    assert hs.element_geometry_counts(_FakeMixedGrid()) == {"cube": 2, "simplex": 2}


def test_dg_dof_count_on_mixed_grid_sums_per_geometry():
    # order 1: two quads (Q_1: 4 DoFs) + two triangles (P_1: 3 DoFs) = 2*4 + 2*3 = 14
    assert hs.dg_dof_count_on_mixed_grid(_FakeMixedGrid(), order=1) == 14
    # order 0: one DoF per element regardless of geometry -> 4
    assert hs.dg_dof_count_on_mixed_grid(_FakeMixedGrid(), order=0) == 4


# --- WP4 (#320): vector-valued CG DoF-count formula ----------------------------------------


@pytest.mark.parametrize(
    ("order", "num_elements", "expected"),
    [
        (1, (2, 2), 9),  # P_1 nodes on a 2x2 quad grid: 3 per axis
        (2, (2, 2), 25),  # P_2: 2*2+1 = 5 per axis
        (1, (2, 2, 2), 27),  # 3d: 3 per axis, three axes
    ],
)
def test_cg_scalar_dof_count(order, num_elements, expected):
    assert hs.cg_scalar_dof_count(order, num_elements) == expected


@pytest.mark.parametrize("dim_range", [1, 2, 3])
@pytest.mark.parametrize(
    ("order", "num_elements"),
    [(1, (2, 2)), (2, (2, 2)), (1, (2, 2, 2)), (3, (1, 1))],
)
def test_cg_vector_dof_count_is_dim_range_times_scalar(order, num_elements, dim_range):
    assert hs.cg_vector_dof_count(order, num_elements, dim_range) == dim_range * hs.cg_scalar_dof_count(
        order, num_elements
    )


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
