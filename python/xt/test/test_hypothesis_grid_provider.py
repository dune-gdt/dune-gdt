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
        # every cube grid -- the structured YASP grid and the unstructured, nonconforming ALU
        # cube grid alike -- refines each cell into 2^d children under one global refinement
        assert grid.size(0) == before * 2**spec.dim
    else:
        # the ALU simplex grids refine by bisection (conforming variant) or regular refinement
        # (nonconforming variant), so one global refinement step only guarantees growth; the
        # exact factor is a grid implementation detail
        assert grid.size(0) > before


# The codim-1 intersection-index helpers are guarded with requirements_not_met on non-conforming
# grids (see gridprovider.hh), so this partition property is stated only for conforming grids.
@given(spec=grid_specs(max_elements_per_dim=3, conforming_only=True))
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


@given(spec=grid_specs(max_elements_per_dim=3))
def test_apply_on_each_element_visits_every_element(spec):
    # apply_on_each_element walks the leaf view once (gridprovider.hh), so the number of callback
    # invocations must equal the codim-0 entity count -- exercises the element-walk path that the
    # counting queries above reach only indirectly.
    grid = spec.make_grid()
    visited = []
    grid.apply_on_each_element(lambda _element: visited.append(1))
    assert len(visited) == grid.size(0)


@given(spec=grid_specs(max_elements_per_dim=3))
def test_apply_on_each_intersection_visit_count(spec):
    # apply_on_each_intersection visits, per element, one intersection per codim-1 face. A cube
    # element has exactly 2*dim faces regardless of position, so on an unrefined cube grid the total
    # visit count is deterministic (inner faces are visited once from each side, boundary faces
    # once). For simplex grids the per-element face count differs, so only assert positivity there.
    grid = spec.make_grid()
    visited = []
    grid.apply_on_each_intersection(lambda _intersection: visited.append(1))
    if spec.element == "cube":
        assert len(visited) == spec.expected_num_elements * 2 * spec.dim
    else:
        assert len(visited) > 0


@given(spec=grid_specs())
def test_vertex_centers_lie_in_closed_bounding_box(spec):
    # centers(codim=dim) returns the grid vertices; this drives the codim!=0 branch of centers()
    # (the element-center test above only ever asks for codim 0). Vertices sit on the closed box,
    # so the bounds are inclusive (unlike the strictly-interior element centroids).
    grid = spec.make_grid()
    vertices = np.array(grid.centers(spec.dim), copy=False)
    assert vertices.shape == (grid.size(spec.dim), spec.dim)
    lower = np.asarray(spec.lower_left)
    upper = np.asarray(spec.upper_right)
    slack = 1e-12 * np.maximum(1.0, np.abs(upper - lower) + np.abs(lower))
    assert (vertices >= lower - slack).all()
    assert (vertices <= upper + slack).all()


@given(spec=grid_specs(max_elements_per_dim=3))
def test_max_level_increases_under_global_refine(spec):
    # a fresh grid is a single level; global_refine adds at least one (bisection grids may add more
    # than one per step, so this is a monotonicity, not an equality, assertion).
    grid = spec.make_grid()
    before = grid.max_level
    grid.global_refine(1)
    assert grid.max_level > before


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
