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
"""Intersection/coupling bilinear forms driven through ``BilinearForm.apply2``.

Every existing ``apply2`` test (test_bilinear_form_apply2.py, test_hypothesis_operators.py,
test_hypothesis_operator_interface.py) appends only *element* bilinear forms, so the two
intersection loops of the ``BilinearFormAssembler`` never iterate:

  * ``for (auto& data : ...coupling_intersection_data())`` — dune/gdt/operators/bilinear-form.hh:434
  * ``for (auto& data : ...intersection_data())``          — dune/gdt/operators/bilinear-form.hh:460

A plain leaf-view ``BilinearForm``'s ``apply2`` builds a generic ``XT::Grid::Walker`` and walks it,
so the assembler's ``apply_local(intersection, inside, outside)`` — hence both loops above — runs
without any coupling/glued grid, provided intersection forms are appended. This module appends the
already-bound IPDG penalty integrands (a *binary* boundary penalty into a single-sided
``LocalIntersectionIntegralBilinearForm`` feeding the line-460 loop, and a *quaternary* inner
penalty into a ``LocalCouplingIntersectionIntegralBilinearForm`` restricted to inner intersections
feeding the line-434 loop) and checks two properties over hypothesis-generated inputs:

  * each penalty form is symmetric positive semi-definite, so ``apply2(u, u) >= 0``;
  * the combined form (both penalties) equals the sum of the two single-penalty forms — the two
    loops accumulate independently into the same scalar result.

No new bindings are introduced; only the geometry, polynomial order and interpolated function are
hypothesis-generated, on top of the binding-instantiated 2d grid types.
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    grid_specs,
    has_grid,
    polynomials,
)

# The intersection loops need a genuine skeleton (inner + boundary intersections), so this suite
# is 2d-only; skip cleanly on builds that bind no 2d grid at all.
pytestmark = pytest.mark.skipif(
    not has_grid(dim=2), reason="no 2d grid bindings available in this build"
)

GRIDS = grid_specs(dims=(2,), unit_box=True, max_elements_per_dim=2)
ORDERS = st.integers(1, 2)
PENALTIES = st.floats(1.0, 32.0, allow_nan=False, allow_infinity=False)


def _penalty_forms(grid, penalty):
    """(boundary-only, inner-only, combined) IPDG-penalty BilinearForms on ``grid``.

    boundary-only exercises the single-sided ``intersection_data()`` loop (bilinear-form.hh:460),
    inner-only the two-sided ``coupling_intersection_data()`` loop (bilinear-form.hh:434); combined
    holds both.
    """
    from dune.gdt import (
        BilinearForm,
        LocalCouplingIntersectionIntegralBilinearForm,
        LocalIntersectionIntegralBilinearForm,
        LocalIPDGBoundaryPenaltyIntegrand,
        LocalIPDGInnerPenaltyIntegrand,
    )
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import ApplyOnInnerIntersectionsOnce, Dim

    d = grid.dimension
    # IPDG penalty integrands take a matrix-valued (d x d) weight grid function.
    weight = GF(grid, 1, dim_range=(Dim(d), Dim(d)))

    def boundary_term():
        # binary intersection integrand -> single-sided intersection bilinear form
        return LocalIntersectionIntegralBilinearForm(
            LocalIPDGBoundaryPenaltyIntegrand(penalty, weight)
        )

    def inner_term():
        # quaternary intersection integrand -> coupling (two-sided) intersection bilinear form
        return LocalCouplingIntersectionIntegralBilinearForm(
            LocalIPDGInnerPenaltyIntegrand(penalty, weight)
        )

    boundary_only = BilinearForm(grid)
    boundary_only += boundary_term()

    inner_only = BilinearForm(grid)
    # the quaternary integrand needs a valid outside element -> restrict to inner intersections
    inner_only += (inner_term(), ApplyOnInnerIntersectionsOnce(grid))

    combined = BilinearForm(grid)
    combined += boundary_term()
    combined += (inner_term(), ApplyOnInnerIntersectionsOnce(grid))

    return boundary_only, inner_only, combined


def _interpolated(grid, order, poly):
    from dune.gdt import DiscontinuousLagrangeSpace, default_interpolation
    from dune.xt.functions import GridFunction as GF

    # a DG space so the inner-penalty jump term is not structurally zero
    space = DiscontinuousLagrangeSpace(grid, order=order)
    u_h = default_interpolation(GF(grid, poly.to_function()), space)
    # apply2/norm expect a GridFunction, so wrap the DiscreteFunction
    return GF(grid, u_h)


@given(data=st.data(), spec=GRIDS, order=ORDERS, penalty=PENALTIES)
def test_ipdg_penalty_forms_are_positive_semidefinite_and_additive(
    data, spec, order, penalty
):
    grid = spec.make_grid()
    # refine once so there is always a non-trivial skeleton: several inner intersections (feeding
    # the coupling loop) as well as boundary intersections (feeding the single-sided loop).
    grid.global_refine(1)

    poly = data.draw(polynomials(dim=2, max_order=order), label="u")
    u = _interpolated(grid, order, poly)

    boundary_only, inner_only, combined = _penalty_forms(grid, penalty)

    b = boundary_only.apply2(u, u)
    i = inner_only.apply2(u, u)
    c = combined.apply2(u, u)

    # every example crosses into compiled assembly; the scalars are plain python floats
    assert np.isfinite(b) and np.isfinite(i) and np.isfinite(c)

    # penalty forms eta/h * integral(weight [u] . [u]) are symmetric positive semi-definite
    scale = penalty * (abs(b) + abs(i) + abs(c) + 1.0)
    assert b >= -1e-10 * scale
    assert i >= -1e-10 * scale

    # the combined form's two loops accumulate independently into the same scalar: the boundary
    # (line-460) loop contributes exactly b, the coupling (line-434) loop exactly i.
    assert np.isclose(c, b + i, rtol=1e-9, atol=1e-9 * scale)


@given(spec=GRIDS, order=ORDERS, penalty=PENALTIES)
def test_boundary_penalty_is_strictly_positive_for_a_nonzero_boundary_trace(
    spec, order, penalty
):
    """A non-zero constant has a non-zero boundary trace, so the boundary penalty is > 0.

    This pins the single-sided ``intersection_data()`` loop (bilinear-form.hh:460) to a genuinely
    non-trivial result (rather than a vacuously-zero one), while the interior jump of a constant
    DG interpolant is zero, so the inner (coupling) penalty vanishes.
    """
    from dune.gdt import DiscontinuousLagrangeSpace, default_interpolation
    from dune.xt.functions import GridFunction as GF

    grid = spec.make_grid()
    grid.global_refine(1)

    space = DiscontinuousLagrangeSpace(grid, order=order)
    u = GF(grid, default_interpolation(GF(grid, 1.0), space))

    boundary_only, inner_only, _ = _penalty_forms(grid, penalty)

    b = boundary_only.apply2(u, u)
    i = inner_only.apply2(u, u)

    assert b > 0.0
    # a constant is continuous across the skeleton, so its interior jump -- and the inner penalty --
    # is zero up to round-off
    assert abs(i) <= 1e-10 * penalty * (b + 1.0)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
