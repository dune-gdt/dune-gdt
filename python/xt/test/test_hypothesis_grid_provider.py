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
"""Scenario 2: structural invariants of make_cube_grid over generated geometries.

The C++ suite pins these properties for a handful of hard-coded (grid type x dimension)
combinations via TYPED_TEST grid lists; each combination costs a template instantiation in
every test TU. Here the geometry (bounding box, per-axis resolution) and the grid type are
drawn at runtime for the binding-instantiated grid types, so shrinking produces the smallest
grid that violates a property instead of a fixed fixture failing wholesale.
"""

import numpy as np
import pytest
from hypothesis import given

from dune.xt.test.hypothesis_strategies import GRID_COMBINATIONS, grid_specs

# Skip cleanly on builds that bind no grids at all (nothing to draw from otherwise).
pytestmark = pytest.mark.skipif(
    not GRID_COMBINATIONS, reason="no grid bindings available in this build"
)


@given(spec=grid_specs())
def test_element_and_vertex_counts(spec):
    grid = spec.make_grid()
    assert grid.dimension == spec.dim
    assert grid.size(0) == spec.expected_num_elements
    assert grid.size(spec.dim) == spec.expected_num_vertices


@given(spec=grid_specs())
def test_element_centers_lie_inside_the_bounding_box(spec):
    grid = spec.make_grid()
    centers = np.array(grid.centers(0), copy=False)
    assert centers.shape == (grid.size(0), spec.dim)
    lower = np.asarray(spec.lower_left)
    upper = np.asarray(spec.upper_right)
    # strict interiority holds for centroids of non-degenerate cells; allow round-off slack
    slack = 1e-12 * np.maximum(1.0, np.abs(upper - lower) + np.abs(lower))
    assert (centers > lower - slack).all()
    assert (centers < upper + slack).all()


@given(spec=grid_specs(max_elements_per_dim=3))
def test_global_refine_scales_element_count(spec):
    grid = spec.make_grid()
    before = grid.size(0)
    grid.global_refine(1)
    if spec.element == "cube":
        # structured cube grids refine each cell into 2^d children
        assert grid.size(0) == before * 2**spec.dim
    else:
        # the conforming ALU simplex grids refine by bisection, so one global refinement
        # step only guarantees growth; the exact factor is a grid implementation detail
        assert grid.size(0) > before


@given(spec=grid_specs(max_elements_per_dim=3))
def test_boundary_and_inner_intersection_indices_partition(spec):
    grid = spec.make_grid()
    boundary = set(grid.boundary_intersection_indices())
    n_boundary = len(boundary)
    assert n_boundary > 0
    if spec.expected_num_elements > 1:
        # single-element grids have no inner intersections, and iterating the *empty*
        # index vector segfaults with the current bindings (iterating any empty LA vector
        # does, see test_hypothesis_la_container.py::test_iterating_empty_vector -- found
        # by hypothesis shrinking this very test); guard until that is fixed
        inner = set(grid.inner_intersection_indices())
        # every codim-1 entity of the (boundary-touching) structured box shows up either as
        # a boundary intersection or as an inner one seen from both neighbours, never both
        assert boundary.isdisjoint(inner)
    if spec.element == "cube":
        expected_boundary = 2 * sum(
            int(np.prod([n for jj, n in enumerate(spec.num_elements) if jj != ii]))
            for ii in range(spec.dim)
        )
        assert n_boundary == expected_boundary


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
