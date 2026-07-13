# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-gdt/dune-gdt
# Copyright 2010-2026 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~
"""WP3 (#320): mixed-element and prism grids via UGGrid.

The structured grid_specs used by the other hypothesis suites always carry a single geometry type
per grid, so the per-geometry-type DoF bookkeeping of the DG mapper is never exercised on a mesh
that mixes cubes and simplices. UGGrid can, and make_mixed_grid / make_prism_grid (mirroring the
C++ fixtures in dune/gdt/test/spaces/base.hh) build exactly such meshes. This module pins

  * the per-geometry-type element counts (the fixtures have a fixed topology), and
  * the DG DoF-count property generalized to mixed grids: the global DoF count is the sum over
    elements of the local (per-geometry) Lagrange DoF count, at any refinement level.
"""

import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    MIXED_GRID_ELEMENTS,
    PRISM_GRID_ELEMENTS,
    dg_dof_count_on_mixed_grid,
    element_geometry_counts,
    has_uggrid,
    make_mixed_grid_provider,
    make_prism_grid_provider,
)

# UGGrid is an optional dependency; without it there are no mixed/prism factories to test.
pytestmark = pytest.mark.skipif(
    not has_uggrid(), reason="no UGGrid bindings available in this build"
)


@pytest.mark.parametrize("dim", [2, 3])
def test_mixed_grid_has_the_expected_geometry_types(dim):
    grid = make_mixed_grid_provider(dim)
    counts = element_geometry_counts(grid)
    assert counts == MIXED_GRID_ELEMENTS[dim]
    assert sum(counts.values()) == grid.size(0)
    # a genuinely mixed mesh: more than one geometry type is present
    assert len([c for c in counts.values() if c]) > 1


def test_prism_grid_is_a_single_prism():
    grid = make_prism_grid_provider()
    counts = element_geometry_counts(grid)
    assert counts == PRISM_GRID_ELEMENTS[3]
    assert sum(counts.values()) == grid.size(0)


@given(
    dim=st.sampled_from([2, 3]), order=st.integers(0, 2), refinements=st.integers(0, 1)
)
def test_dg_dof_count_generalizes_to_mixed_grids(dim, order, refinements):
    from dune.gdt import DiscontinuousLagrangeSpace

    grid = make_mixed_grid_provider(dim, num_refinements=refinements)
    space = DiscontinuousLagrangeSpace(grid, order=order)
    # the DG DoF count is per element and per geometry type: cubes contribute Q_k, simplices P_k;
    # summing over the actually present elements must reproduce the mapper's global count exactly
    assert space.num_DoFs == dg_dof_count_on_mixed_grid(grid, order)


@given(refinements=st.integers(0, 1))
def test_refinement_preserves_the_mixed_geometry_types(refinements):
    grid = make_mixed_grid_provider(2, num_refinements=refinements)
    counts = element_geometry_counts(grid)
    # refinement multiplies the element counts but never introduces an unclassifiable geometry
    assert "unknown" not in counts
    assert set(counts) == set(MIXED_GRID_ELEMENTS[2])


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
