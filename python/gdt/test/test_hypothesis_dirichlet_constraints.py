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
"""Dirichlet constraints: boundary-DoF detection and the matrix/vector apply combinations.

dune/gdt/tools/dirichlet-constraints.hh gathers the boundary DoFs of a space (walking the grid with
an ``AllDirichletBoundaryInfo``, which flags every boundary intersection as Dirichlet -- the first
operand of the ``DirichletBoundary() || process-boundary`` test in ``apply_local``) and then edits an
assembled system so the constrained rows become identities (or are merely cleared). The C++ test
(dune/gdt/test/misc/tools_dirichlet_constraints.cc) fixes a single 4x4 grid and only the default
``apply(matrix, vector)``; here the four ``(only_clear, ensure_symmetry)`` combinations of the
matrix-and-vector ``apply`` are swept over hypothesis-generated grids, pinning the properties every
combination shares:

  * every combination zeroes the right-hand side at the constrained DoFs;
  * the identity variants (``only_clear=False``) additionally turn the constrained rows into the
    identity, so ``(matrix @ x)[dof] == x[dof]`` there.
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    GRID_COMBINATIONS,
    grid_specs,
)

# Skip cleanly on builds that bind no grids at all (nothing to draw from otherwise).
pytestmark = pytest.mark.skipif(
    not GRID_COMBINATIONS, reason="no grid bindings available in this build"
)


def _laplace_matrix_operator(grid, space, kappa):
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        LocalLaplaceIntegrand,
        MatrixOperator,
        make_element_sparsity_pattern,
    )
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import Dim

    d = grid.dimension
    form = BilinearForm(grid)
    form += LocalElementIntegralBilinearForm(
        LocalLaplaceIntegrand(GF(grid, kappa, dim_range=(Dim(d), Dim(d))))
    )
    op = MatrixOperator(
        grid,
        source_space=space,
        range_space=space,
        sparsity_pattern=make_element_sparsity_pattern(space),
    )
    op.append(form)
    op.assemble()
    return op


def _constraints(grid, space):
    from dune.gdt import DirichletConstraints
    from dune.xt.grid import AllDirichletBoundaryInfo, Walker

    # AllDirichletBoundaryInfo makes boundary_info.type(intersection) == DirichletBoundary() on every
    # boundary intersection, exercising the first operand of the || in apply_local.
    boundary_info = AllDirichletBoundaryInfo(grid)
    constraints = DirichletConstraints(boundary_info, space)
    walker = Walker(grid)
    walker.append(constraints)
    walker.walk()
    return constraints


def _make_vector(size, value):
    from dune.xt.la import IstlVector

    return IstlVector(size, float(value))


GRIDS = grid_specs(unit_box=True, max_elements_per_dim=3)
KAPPAS = st.floats(1e-2, 1e2, allow_nan=False, allow_infinity=False)
ORDERS = st.integers(1, 2)


@given(spec=GRIDS, order=ORDERS)
def test_constraints_find_boundary_dofs(spec, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    constraints = _constraints(grid, space)

    dofs = list(constraints.dirichlet_DoFs)
    # every closed structured domain has a boundary, so some DoFs are constrained, but not all of
    # them (the interior is unconstrained for any grid with an interior node).
    assert len(dofs) > 0
    assert len(dofs) <= space.num_DoFs


@given(
    data=st.data(),
    spec=GRIDS,
    kappa=KAPPAS,
    order=ORDERS,
    only_clear=st.booleans(),
    ensure_symmetry=st.booleans(),
)
def test_matrix_and_vector_apply_zeroes_rhs_for_every_combination(
    data, spec, kappa, order, only_clear, ensure_symmetry
):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_matrix_operator(grid, space, kappa)
    constraints = _constraints(grid, space)
    dofs = list(constraints.dirichlet_DoFs)

    n = space.num_DoFs
    rhs = _make_vector(n, 1.0)
    # apply edits the matrix in place and always sets the constrained rhs entries to zero, whichever
    # of the four (only_clear, ensure_symmetry) branches is taken.
    constraints.apply(
        op.matrix, rhs, only_clear=only_clear, ensure_symmetry=ensure_symmetry
    )

    rhs_np = np.asarray(rhs, dtype=float)
    assert np.allclose(rhs_np[dofs], 0.0)

    if not only_clear:
        # the identity variants turn every constrained row into a unit row, so the constrained
        # component of matrix @ x is exactly x there.
        x_vals = np.array(
            data.draw(
                st.lists(
                    st.floats(-1.0, 1.0, allow_nan=False, allow_infinity=False),
                    min_size=n,
                    max_size=n,
                ),
                label="x",
            ),
            dtype=float,
        )
        from dune.xt.la import IstlVector

        x = IstlVector(n, 0.0)
        for ii, value in enumerate(x_vals):
            x.set_entry(ii, float(value))
        y = IstlVector(n, 0.0)
        op.matrix.mv(x, y)
        y_np = np.asarray(y, dtype=float)
        scale = np.abs(x_vals).max() + 1.0
        assert np.allclose(y_np[dofs], x_vals[dofs], atol=1e-10 * scale)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
