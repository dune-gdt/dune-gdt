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
"""Scenario 4: mathematical properties of default_interpolation on generated polynomials.

The classical exactness property -- Lagrange interpolation of order k reproduces every
polynomial of total degree <= k -- is checked for *random* polynomials with random
coefficients instead of the handful of hand-picked expressions in the C++ interpolation
tests (dune/gdt/test/interpolations/). The custom `polynomials` strategy keeps the
coefficient map in Python, so the second property (linearity of the interpolation operator,
I[a*p + q] == a*I[p] + I[q] exactly on DoF vectors) can build the combined polynomial
symbolically and compare against the C++-computed linear combination.
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    GRID_COMBINATIONS,
    grid_specs,
    polynomials,
)

# Skip cleanly on builds that bind no grids at all (nothing to draw from otherwise).
pytestmark = pytest.mark.skipif(
    not GRID_COMBINATIONS, reason="no grid bindings available in this build"
)


def _l2_norm2(grid, grid_function):
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        LocalElementProductIntegrand,
    )
    from dune.xt.functions import GridFunction as GF

    form = BilinearForm(grid)
    form += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))
    return form.apply2(grid_function, grid_function)


@given(
    data=st.data(),
    # unit_box: polynomial values on wildly scaled domains overflow the exactness tolerance
    # for reasons of floating point conditioning, not interpolation correctness
    spec=grid_specs(unit_box=True, max_elements_per_dim=3),
    order=st.integers(1, 2),
)
def test_interpolation_reproduces_polynomials(data, spec, order):
    from dune.gdt import ContinuousLagrangeSpace, default_interpolation
    from dune.xt.functions import GridFunction as GF

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    poly = data.draw(polynomials(dim=spec.dim, max_order=order))

    interpolant = default_interpolation(GF(grid, poly.to_function()), space)

    diff = GF(grid, GF(grid, interpolant) - GF(grid, poly.to_function()))
    err2 = _l2_norm2(grid, diff)
    ref2 = _l2_norm2(grid, GF(grid, poly.to_function()))
    assert err2 <= 1e-20 * max(ref2, 1.0)


@given(
    data=st.data(),
    spec=grid_specs(unit_box=True, max_elements_per_dim=3),
    order=st.integers(1, 2),
    scalar=st.floats(-100.0, 100.0, allow_nan=False, allow_infinity=False),
)
def test_interpolation_is_linear_on_dof_vectors(data, spec, order, scalar):
    from dune.gdt import ContinuousLagrangeSpace, default_interpolation
    from dune.xt.functions import GridFunction as GF

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    p = data.draw(polynomials(dim=spec.dim, max_order=order))
    q = data.draw(polynomials(dim=spec.dim, max_order=order))
    combined = p.linear_combination(scalar, q)  # scalar*p + q, exact on coefficients

    dofs_p = np.array(
        default_interpolation(GF(grid, p.to_function()), space).dofs.vector, copy=False
    )
    dofs_q = np.array(
        default_interpolation(GF(grid, q.to_function()), space).dofs.vector, copy=False
    )
    dofs_combined = np.array(
        default_interpolation(GF(grid, combined.to_function()), space).dofs.vector,
        copy=False,
    )

    scale = np.abs(scalar * dofs_p) + np.abs(dofs_q) + 1.0
    assert np.allclose(
        dofs_combined, scalar * dofs_p + dofs_q, rtol=0.0, atol=1e-10 * scale.max()
    )


@given(
    data=st.data(),
    spec=grid_specs(unit_box=True, max_elements_per_dim=3),
    order=st.integers(1, 2),
)
def test_interpolation_is_a_projection(data, spec, order):
    """Interpolating an interpolant must be the identity on the DoF vector."""
    from dune.gdt import ContinuousLagrangeSpace, default_interpolation
    from dune.xt.functions import GridFunction as GF

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    poly = data.draw(polynomials(dim=spec.dim, max_order=order))

    once = default_interpolation(GF(grid, poly.to_function()), space)
    twice = default_interpolation(GF(grid, once), space)

    dofs_once = np.array(once.dofs.vector, copy=False)
    dofs_twice = np.array(twice.dofs.vector, copy=False)
    assert np.allclose(
        dofs_twice, dofs_once, rtol=1e-13, atol=1e-13 * (np.abs(dofs_once).max() + 1.0)
    )


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
