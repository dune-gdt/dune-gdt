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
"""Geometric- and functional-analytic invariants of the grid-quality estimators.

The estimators in dune/gdt/tools/grid-quality-estimates.hh return mesh-dependent constants that enter
inverse and trace inequalities. The C++ counterpart (dune/gdt/test/misc/tools_grid_quality_estimates.cc)
pins their value on a single fixed 4x4 / 2x2 YASP square; here the same three functions are exercised across
the whole hypothesis-generated (grid geometry x element type x order) product, and the properties asserted are
the ones that hold independently of the concrete mesh:

  * estimate_element_to_intersection_equivalence_constant is a *ratio* of element and intersection diameters, so
    it is invariant under a uniform scaling of the domain and under uniform refinement (both leave the mesh
    self-similar), and strictly positive in every dimension;
  * the eigen-solver-based estimate_inverse_inequality_constant / estimate_combined_inverse_trace_inequality_constant
    are finite and strictly positive wherever the generalized eigen-solver dependency (LAPACKE) is available.
"""

import functools
import math

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


def _scaled_spec(spec, factor):
    """A geometrically similar copy of spec, with the bounding box scaled about the origin by factor."""
    from dune.xt.test.hypothesis_strategies import GridSpec

    return GridSpec(
        dim=spec.dim,
        element=spec.element,
        lower_left=tuple(factor * x for x in spec.lower_left),
        upper_right=tuple(factor * x for x in spec.upper_right),
        num_elements=spec.num_elements,
        impl=spec.impl,
    )


@functools.lru_cache(maxsize=1)
def _inverse_inequality_available():
    """Whether the generalized eigen-solver backing the inverse/trace estimators is present (LAPACKE).

    Mirrors the ``XT::Common::Lapacke::available()`` guard of the C++ test: on a build without LAPACKE both
    estimators throw, and the two properties below are skipped rather than reported as failures.
    """
    if not GRID_COMBINATIONS:
        return False
    try:
        from dune.gdt import (
            ContinuousLagrangeSpace,
            estimate_inverse_inequality_constant,
        )
        from dune.xt.common import DuneError
        from dune.xt.test.hypothesis_strategies import GridSpec

        grid = GridSpec(
            dim=2,
            element="cube",
            lower_left=(0.0, 0.0),
            upper_right=(1.0, 1.0),
            num_elements=(2, 2),
        ).make_grid()
        space = ContinuousLagrangeSpace(grid, order=1)
        estimate_inverse_inequality_constant(space)
        return True
    except DuneError:
        return False


_needs_eigen_solver = pytest.mark.skipif(
    not _inverse_inequality_available(),
    reason="the generalized eigen-solver dependency (LAPACKE) is unavailable in this build",
)


# the C++ default intersection_diameter callback special-cases d == 1 (averaging the two adjacent element
# diameters, or using the boundary element's own diameter) precisely to avoid the degenerate "intersections
# are points of diameter 0" case, so 1d is exercised on equal footing with 2d/3d here.
@given(spec=grid_specs(dims=(1, 2, 3), max_elements_per_dim=3))
def test_element_to_intersection_equivalence_constant_is_positive_and_finite(spec):
    from dune.gdt import estimate_element_to_intersection_equivalence_constant

    constant = estimate_element_to_intersection_equivalence_constant(spec.make_grid())
    assert math.isfinite(constant)
    assert constant > 0.0


@given(
    spec=grid_specs(dims=(1, 2, 3), max_elements_per_dim=3),
    factor=st.floats(0.25, 4.0, allow_nan=False, allow_infinity=False),
)
def test_equivalence_constant_is_scale_invariant(spec, factor):
    """The constant is a ratio of diameters, so scaling the whole domain leaves it unchanged."""
    from dune.gdt import estimate_element_to_intersection_equivalence_constant

    reference = estimate_element_to_intersection_equivalence_constant(spec.make_grid())
    scaled = estimate_element_to_intersection_equivalence_constant(
        _scaled_spec(spec, factor).make_grid()
    )
    assert scaled == pytest.approx(reference, rel=1e-12, abs=1e-12)


# cube elements refine self-similarly (each element splits into 2^d congruent children); simplicial
# bisection does not preserve element shape, so the diameter ratio is only invariant on cube grids.
@given(spec=grid_specs(dims=(1, 2, 3), elements=("cube",), max_elements_per_dim=2))
def test_equivalence_constant_is_refinement_invariant(spec):
    """Uniform refinement keeps a cube mesh self-similar, hence keeps the (diameter-ratio) constant."""
    from dune.gdt import estimate_element_to_intersection_equivalence_constant

    grid = spec.make_grid()
    coarse = estimate_element_to_intersection_equivalence_constant(grid)
    grid.global_refine(1)
    refined = estimate_element_to_intersection_equivalence_constant(grid)
    assert refined == pytest.approx(coarse, rel=1e-12, abs=1e-12)


@_needs_eigen_solver
@given(spec=grid_specs(max_elements_per_dim=3), order=st.integers(1, 2))
def test_inverse_inequality_constants_are_positive_and_finite(spec, order):
    from dune.gdt import (
        ContinuousLagrangeSpace,
        estimate_combined_inverse_trace_inequality_constant,
        estimate_inverse_inequality_constant,
    )

    space = ContinuousLagrangeSpace(spec.make_grid(), order=order)
    inverse = estimate_inverse_inequality_constant(space)
    combined = estimate_combined_inverse_trace_inequality_constant(space)
    assert math.isfinite(inverse) and inverse > 0.0
    assert math.isfinite(combined) and combined > 0.0


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
