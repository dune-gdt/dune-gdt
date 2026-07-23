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
"""Local forms/operators appended to operators with an element filter, and the operator ``linear`` flag.

Two partial branches in the operator layer are only reachable by appending local pieces that the
existing operator tests never construct -- a non-trivial element filter, and a *nonlinear* local
operator:

  * dune/gdt/operators/matrix.hh:442 -- ``if (filter.contains(assembly_grid_view_, element))`` in
    ``MatrixOperator::apply_local``. Every existing assembly appends element bilinear forms without a
    filter (the default ``ApplyOnAllElements``), so the ``if`` is only ever taken. A ``BilinearForm``
    carrying a real element filter, appended and assembled, drives both sides.
  * dune/gdt/operators/operator.hh:225 and :259 -- ``linear_ = linear_ && local_operator.linear()``
    in the local-element (no-filter) and local-intersection (with-filter) ``operator+=`` overloads.
    Only advection operators report ``linear() == true`` and none are bound as *local* operators, so
    every Python-constructible local operator is nonlinear; appending one flips ``Operator.linear``
    from its initial ``True`` to ``False`` -- the short-circuit's false side.

The element filter used is ``ApplyOnBoundaryElements`` (true on boundary cells, false on interior
ones) together with the trivial ``ApplyOnAllElements`` / ``ApplyOnNoElements`` bookends, all bound in
dune.xt.grid. No new bindings are introduced.

(The sibling ``forward-operator.hh:318`` filter check -- the same ``if (filter.contains(...))`` inside
the ``ForwardOperatorAssembler`` that ``Operator::apply`` walks -- would be reached by applying a
filtered local element indicator operator through ``Operator.apply``. That grid-walking
``Operator.apply``-with-a-local-operator path is not exercised by any C++ test and is left out here;
the appending is fine, only the walk is untested.)
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    GRID_COMBINATIONS,
    grid_specs,
)

pytestmark = pytest.mark.skipif(
    not GRID_COMBINATIONS, reason="no grid bindings available in this build"
)

GRIDS = grid_specs(unit_box=True, max_elements_per_dim=3)
ORDERS = st.integers(1, 2)
WEIGHTS = st.floats(1e-1, 1e1, allow_nan=False, allow_infinity=False)
PENALTIES = st.floats(1.0, 32.0, allow_nan=False, allow_infinity=False)


def _make_vector(size, values):
    from dune.xt.la import IstlVector

    vec = IstlVector(size, 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def _probe_values(data, num_dofs, label):
    entry = st.floats(
        -1.0, 1.0, allow_nan=False, allow_infinity=False, allow_subnormal=False
    )
    return np.array(
        data.draw(st.lists(entry, min_size=num_dofs, max_size=num_dofs), label=label),
        dtype=float,
    )


def _quadratic_form(op, x_vals):
    """x . (M x) for the assembled matrix operator ``op``, as a plain float."""
    x = _make_vector(len(x_vals), x_vals)
    return float(np.dot(x_vals, np.asarray(op.apply(x), dtype=float)))


def _mass_matrix_operator(grid, space, weight, element_filter=None):
    """A scalar mass ``MatrixOperator``; the product integrand is optionally element-filtered."""
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        LocalElementProductIntegrand,
        MatrixOperator,
        make_element_sparsity_pattern,
    )
    from dune.xt.functions import GridFunction as GF

    local_form = LocalElementIntegralBilinearForm(
        LocalElementProductIntegrand(GF(grid, weight))
    )
    form = BilinearForm(grid)
    if element_filter is None:
        form += local_form
    else:
        form += (local_form, element_filter)

    op = MatrixOperator(
        grid,
        source_space=space,
        range_space=space,
        sparsity_pattern=make_element_sparsity_pattern(space),
    )
    op.append(form)
    op.assemble()
    return op


@given(data=st.data(), spec=GRIDS, order=ORDERS, weight=WEIGHTS)
def test_matrix_operator_element_filter_selects_a_positive_semidefinite_subassembly(
    data, spec, order, weight
):
    from dune.gdt import ContinuousLagrangeSpace
    from dune.xt.grid import (
        ApplyOnAllElements,
        ApplyOnBoundaryElements,
        ApplyOnNoElements,
    )

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    n = space.num_DoFs
    x_vals = _probe_values(data, n, "x")

    q_unfiltered = _quadratic_form(_mass_matrix_operator(grid, space, weight), x_vals)
    q_all = _quadratic_form(
        _mass_matrix_operator(grid, space, weight, ApplyOnAllElements(grid)), x_vals
    )
    q_none = _quadratic_form(
        _mass_matrix_operator(grid, space, weight, ApplyOnNoElements(grid)), x_vals
    )
    q_boundary = _quadratic_form(
        _mass_matrix_operator(grid, space, weight, ApplyOnBoundaryElements(grid)),
        x_vals,
    )

    scale = weight * n + 1.0
    tol = 1e-9 * scale

    # ApplyOnAllElements is exactly the (default) unfiltered assembly: filter.contains always true.
    assert np.isclose(q_all, q_unfiltered, rtol=1e-9, atol=tol)
    # ApplyOnNoElements assembles nothing: filter.contains always false -> zero matrix.
    assert abs(q_none) <= tol
    # ApplyOnBoundaryElements keeps only the boundary elements' (positive semi-definite) mass
    # contributions, so 0 <= x.(M_boundary x) <= x.(M_all x): the interior remainder is PSD too.
    assert q_boundary >= -tol
    assert q_boundary <= q_all + tol


@given(spec=GRIDS, order=ORDERS, weight=WEIGHTS)
def test_operator_linear_flag_flips_for_a_nonlinear_local_element_operator(
    spec, order, weight
):
    """operator.hh:225 -- ``linear_ = linear_ && local_operator.linear()`` (element, no filter)."""
    from dune.gdt import (
        ContinuousLagrangeSpace,
        LocalElementBilinearFormIndicatorOperator,
        LocalElementIntegralBilinearForm,
        LocalElementProductIntegrand,
        Operator,
    )
    from dune.xt.functions import GridFunction as GF

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = Operator(grid, source_space=space, range_space=space)

    # a freshly built operator is linear (linear_ initialised to true)
    assert op.linear is True

    indicator = LocalElementBilinearFormIndicatorOperator(
        LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, weight)))
    )
    # the indicator operator does not override the interface default linear() == false
    assert indicator.linear is False

    op += indicator
    # linear_ && false -> false: the short-circuit's false side (operator.hh:225)
    assert op.linear is False


@given(spec=GRIDS, order=ORDERS, penalty=PENALTIES)
def test_operator_linear_flag_flips_for_a_nonlinear_local_intersection_operator(
    spec, order, penalty
):
    """operator.hh:259 -- ``linear_ = linear_ && local_operator.linear()`` (intersection, filtered)."""
    from dune.gdt import (
        ContinuousLagrangeSpace,
        LocalIntersectionBilinearFormIndicatorOperator,
        LocalIntersectionIntegralBilinearForm,
        LocalIPDGBoundaryPenaltyIntegrand,
        Operator,
    )
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import ApplyOnAllIntersections, Dim

    grid = spec.make_grid()
    d = grid.dimension
    space = ContinuousLagrangeSpace(grid, order=order)
    op = Operator(grid, source_space=space, range_space=space)
    assert op.linear is True

    weight = GF(grid, 1, dim_range=(Dim(d), Dim(d)))
    intersection_form = LocalIntersectionIntegralBilinearForm(
        LocalIPDGBoundaryPenaltyIntegrand(penalty, weight)
    )
    indicator = LocalIntersectionBilinearFormIndicatorOperator(intersection_form)
    assert indicator.linear is False

    # append through the (operator, intersection-filter) tuple overload -> operator.hh:259
    op += (indicator, ApplyOnAllIntersections(grid))
    assert op.linear is False


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
