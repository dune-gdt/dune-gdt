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
"""Scenario 5: algebraic properties of assembled MatrixOperators under generated coefficients.

Drives the full C++ assembly stack (BilinearForm -> LocalElementIntegralBilinearForm ->
LocalLaplaceIntegrand/LocalElementProductIntegrand -> grid walk -> IstlSparseMatrix) and
checks properties every correct assembly must satisfy, for hypothesis-generated diffusion
constants, grid geometries and probe vectors:

  * the Laplace stiffness matrix is symmetric and positive semi-definite,
  * constants span its kernel (A @ 1 == 0),
  * assembly is homogeneous in the coefficient (assembling kappa*f equals kappa*(assembling f)),
  * the weighted L2 matrix is positive definite for strictly positive weights.

The C++ operator tests (dune/gdt/test/operators/operators_matrix.cc etc.) fix all of these
inputs at compile time; here only the five binding-instantiated grid types are fixed.
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import grid_specs


def _assemble(grid, space, integrand):
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        MatrixOperator,
        make_element_sparsity_pattern,
    )

    form = BilinearForm(grid)
    form += LocalElementIntegralBilinearForm(integrand)
    op = MatrixOperator(
        grid,
        source_space=space,
        range_space=space,
        sparsity_pattern=make_element_sparsity_pattern(space),
    )
    op.append(form)
    op.assemble()
    return op.matrix


def _laplace_matrix(grid, space, kappa):
    from dune.gdt import LocalLaplaceIntegrand
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import Dim

    d = grid.dimension
    return _assemble(
        grid, space, LocalLaplaceIntegrand(GF(grid, kappa, dim_range=(Dim(d), Dim(d))))
    )


def _l2_matrix(grid, space, weight):
    from dune.gdt import LocalElementProductIntegrand
    from dune.xt.functions import GridFunction as GF

    return _assemble(grid, space, LocalElementProductIntegrand(GF(grid, weight)))


def _make_vector(size, values):
    from dune.xt.la import IstlVector

    vec = IstlVector(size, 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def _mv(matrix, vec):
    from dune.xt.la import IstlVector

    result = IstlVector(len(vec), 0.0)
    matrix.mv(vec, result)
    return result


def _random_probe_vectors(data, num_dofs, count):
    # probe vectors are drawn per-entry by hypothesis (not np.random), so shrinking also
    # simplifies the probes of a failing example. allow_subnormal=False: a probe like
    # [0.0, 2.2e-313] (found by hypothesis) has a subnormal sup norm whose reciprocal
    # overflows to inf, turning the normalization below into nan entries
    entry = st.floats(
        -1.0, 1.0, allow_nan=False, allow_infinity=False, allow_subnormal=False
    )
    return [
        _make_vector(
            num_dofs,
            data.draw(
                st.lists(entry, min_size=num_dofs, max_size=num_dofs),
                label=f"probe {ii}",
            ),
        )
        for ii in range(count)
    ]


GRIDS = grid_specs(unit_box=True, max_elements_per_dim=3)
KAPPAS = st.floats(1e-3, 1e3, allow_nan=False, allow_infinity=False)
ORDERS = st.integers(1, 2)


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_laplace_matrix_is_symmetric_and_psd(data, spec, kappa, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    mat = _laplace_matrix(grid, space, kappa)

    x, y = _random_probe_vectors(data, space.num_DoFs, 2)
    ax, ay = _mv(mat, x), _mv(mat, y)
    scale = kappa * space.num_DoFs
    # <y, Ax> == <x, Ay> iff A is symmetric (up to round-off of the two accumulations)
    assert y.dot(ax) == pytest.approx(x.dot(ay), rel=1e-10, abs=1e-10 * scale)
    assert x.dot(ax) >= -1e-12 * scale


@given(spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_constants_are_in_the_laplace_kernel(spec, kappa, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    mat = _laplace_matrix(grid, space, kappa)

    # the coefficient vector of the constant-one function in a Lagrange basis is all-ones,
    # and grad(1) == 0, so the stiffness matrix must annihilate it
    ones = _make_vector(space.num_DoFs, np.ones(space.num_DoFs))
    residual = _mv(mat, ones)
    assert residual.sup_norm() <= 1e-10 * kappa


@given(
    data=st.data(),
    spec=GRIDS,
    kappa=KAPPAS,
    scaling=st.floats(1e-2, 1e2, allow_nan=False, allow_infinity=False),
    order=ORDERS,
)
def test_assembly_is_homogeneous_in_the_coefficient(data, spec, kappa, scaling, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    mat = _laplace_matrix(grid, space, kappa)
    mat_scaled = _laplace_matrix(grid, space, scaling * kappa)

    (x,) = _random_probe_vectors(data, space.num_DoFs, 1)
    ax, ax_scaled = _mv(mat, x), _mv(mat_scaled, x)
    ax.scal(scaling)
    assert ax_scaled.sup_norm() == pytest.approx(
        ax.sup_norm(), rel=1e-10, abs=1e-10 * kappa * scaling
    )
    ax.axpy(-1.0, ax_scaled)
    assert ax.sup_norm() <= 1e-9 * max(1.0, kappa * scaling)


@given(
    data=st.data(),
    spec=GRIDS,
    weight=st.floats(1e-3, 1e3, allow_nan=False, allow_infinity=False),
    order=ORDERS,
)
def test_weighted_l2_matrix_is_positive_definite(data, spec, weight, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    mat = _l2_matrix(grid, space, weight)

    (x,) = _random_probe_vectors(data, space.num_DoFs, 1)
    if x.sup_norm() == 0.0:
        return  # positive definiteness says nothing about the zero vector
    # normalize the probe: for probes like [0.0, 5e-324] (found by hypothesis) the squares
    # underflow to 0.0, which is a property of double arithmetic, not of the matrix
    x.scal(1.0 / x.sup_norm())
    # <x, Mx> == integral of (weight * u_x^2) > 0 for u_x != 0
    assert x.dot(_mv(mat, x)) > 0.0


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
