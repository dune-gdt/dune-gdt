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
"""BoundaryDetectorFunctor: counting boundary intersections of a walked grid.

dune/xt/grid/functors/boundary-detector.hh counts, while a Walker walks a grid view, the
intersections whose ``BoundaryInfo`` type equals a requested ``BoundaryType`` (its ``compute_locally``
returns ``boundary_info.type(intersection) == boundary_type``, accumulated in ``apply_local`` and read
back through ``result``). The C++ test (dune/xt/test/grid/walker.cc::check_boundaries) fixes a single
grid and only the ``AllDirichletBoundaryInfo``/``DirichletBoundary`` pairing; here the grid geometry
and type are drawn by hypothesis and the functor is cross-checked against the grid's own geometry.

The ``All<Kind>BoundaryInfo`` classes all mark exactly the domain-boundary intersections with their
respective ``<Kind>Boundary`` type (alldirichlet.hh / allneumann.hh / allreflecting.hh), so a detector
built from a matching (info, type) pair must report exactly the number of boundary intersections --
which is independently counted here by walking the grid and testing ``intersection.boundary``, and,
for the structured grids, is also known in closed form (GridSpec.expected_num_boundary_intersections).
A detector built from a *mismatched* pair (Dirichlet info, Neumann type) must report zero, exercising
the ``compute_locally`` branch that returns 0 on every intersection.
"""

import pytest
from hypothesis import given

from dune.xt.test.hypothesis_strategies import GRID_COMBINATIONS, grid_specs

# Skip cleanly on builds that bind no grids at all (nothing to draw from otherwise).
pytestmark = pytest.mark.skipif(
    not GRID_COMBINATIONS, reason="no grid bindings available in this build"
)


def _count_boundary_intersections(grid):
    """Independently count the leaf boundary intersections by walking the grid geometry.

    apply_on_each_intersection visits every inner intersection once from each adjacent element and
    every boundary intersection once, so counting the visits with ``intersection.boundary`` set gives
    exactly the number of boundary intersections -- the geometric ground truth the functor must hit.
    """
    visited = []
    grid.apply_on_each_intersection(
        lambda intersection: intersection.boundary and visited.append(1)
    )
    return len(visited)


def _detect(grid, boundary_info, boundary_type):
    """Walk ``grid`` with a BoundaryDetectorFunctor and return its result."""
    from dune.xt.grid import BoundaryDetectorFunctor, Walker

    detector = BoundaryDetectorFunctor(grid, boundary_info, boundary_type)
    walker = Walker(grid)
    walker.append(detector)
    walker.walk()
    return detector.result


@given(spec=grid_specs(max_elements_per_dim=3))
def test_detects_all_boundary_intersections(spec):
    from dune.xt.grid import AllDirichletBoundaryInfo, DirichletBoundary

    grid = spec.make_grid()
    detected = _detect(grid, AllDirichletBoundaryInfo(grid), DirichletBoundary())

    # the detector must agree with the grid's own boundary geometry ...
    assert detected == _count_boundary_intersections(grid)
    # ... and a closed structured box always has some boundary
    assert detected > 0


@given(spec=grid_specs(max_elements_per_dim=3))
def test_detected_count_matches_closed_form(spec):
    from dune.xt.grid import AllDirichletBoundaryInfo, DirichletBoundary

    grid = spec.make_grid()
    detected = _detect(grid, AllDirichletBoundaryInfo(grid), DirichletBoundary())
    assert detected == spec.expected_num_boundary_intersections


@given(spec=grid_specs(max_elements_per_dim=3))
def test_boundary_info_kind_does_not_change_the_count(spec):
    # AllDirichlet/AllNeumann/AllReflecting each tag exactly the boundary intersections with their own
    # BoundaryType, so a detector matched to each must report the same boundary-intersection count.
    from dune.xt.grid import (
        AllDirichletBoundaryInfo,
        AllNeumannBoundaryInfo,
        AllReflectingBoundaryInfo,
        DirichletBoundary,
        NeumannBoundary,
        ReflectingBoundary,
    )

    grid = spec.make_grid()
    dirichlet = _detect(grid, AllDirichletBoundaryInfo(grid), DirichletBoundary())
    neumann = _detect(grid, AllNeumannBoundaryInfo(grid), NeumannBoundary())
    reflecting = _detect(grid, AllReflectingBoundaryInfo(grid), ReflectingBoundary())

    assert dirichlet == neumann == reflecting


@given(spec=grid_specs(max_elements_per_dim=3))
def test_mismatched_boundary_type_detects_nothing(spec):
    # AllDirichletBoundaryInfo never reports NeumannBoundary, so compute_locally returns 0 on every
    # intersection and the accumulated result is exactly zero.
    from dune.xt.grid import AllDirichletBoundaryInfo, NeumannBoundary

    grid = spec.make_grid()
    assert _detect(grid, AllDirichletBoundaryInfo(grid), NeumannBoundary()) == 0


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
