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
"""Operator-interface arithmetic: subtraction, jacobian/invert options and the zero jacobian.

test_hypothesis_operator_interface.py exercises ``apply``, scalar multiplication, addition and
negation of assembled MatrixOperators. The remaining branches of dune/gdt/operators/interfaces.hh
are only reached by the *other* sign of the arithmetic ternaries and by the option-selection
machinery, so they are covered here:

  * operator **subtraction** ``A - B`` and ``A - vector`` flip the ``add ? "+" : "-"`` ternary and
    the ``add ? 1. : -1.`` coefficient in ``make_operator_addsub`` (only ``+`` is exercised by the
    existing suite); the results apply as ``A.apply(x) - B.apply(x)`` resp. ``A.apply(x) - vector``;
  * ``jacobian_options(type)`` / ``invert_options(type)`` return the matching options dict for a
    reported type and raise for an unknown one, flipping the ``opts["type"] == type`` match ``if``;
  * a linear combination ``A + vector`` holds a ``ConstantOperator`` (whose reported jacobian option
    is ``{"type": "zero"}``); its ``jacobian`` delegates to that constant summand and takes the
    ``type == "zero"`` early-return branch, while the matrix summand contributes its matrix -- so the
    assembled jacobian of ``A + vector`` equals the matrix of ``A``.

All inputs (grid geometry, order, coefficients, probe vectors) are hypothesis-generated; only the
five binding-instantiated grid types are fixed.
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


def _matrix_operator(grid, space, integrand):
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
    return op


def _empty_matrix_operator(grid, space):
    """A MatrixOperator with an allocated (zero) matrix, suitable as a jacobian target."""
    from dune.gdt import MatrixOperator, make_element_sparsity_pattern

    return MatrixOperator(
        grid,
        source_space=space,
        range_space=space,
        sparsity_pattern=make_element_sparsity_pattern(space),
    )


def _laplace_operator(grid, space, kappa):
    from dune.gdt import LocalLaplaceIntegrand
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import Dim

    d = grid.dimension
    return _matrix_operator(
        grid, space, LocalLaplaceIntegrand(GF(grid, kappa, dim_range=(Dim(d), Dim(d))))
    )


def _l2_operator(grid, space, weight):
    from dune.gdt import LocalElementProductIntegrand
    from dune.xt.functions import GridFunction as GF

    return _matrix_operator(grid, space, LocalElementProductIntegrand(GF(grid, weight)))


def _make_vector(size, values):
    from dune.xt.la import IstlVector

    vec = IstlVector(size, 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def _sup_diff(a, b):
    """sup norm of a - b, without assuming Python-level vector subtraction is bound."""
    diff = _make_vector(len(a), np.zeros(len(a)))
    diff.axpy(1.0, a)
    diff.axpy(-1.0, b)
    return diff.sup_norm()


def _probe_values(data, num_dofs, label):
    entry = st.floats(
        -1.0, 1.0, allow_nan=False, allow_infinity=False, allow_subnormal=False
    )
    return np.array(
        data.draw(st.lists(entry, min_size=num_dofs, max_size=num_dofs), label=label),
        dtype=float,
    )


GRIDS = grid_specs(unit_box=True, max_elements_per_dim=3)
KAPPAS = st.floats(1e-2, 1e2, allow_nan=False, allow_infinity=False)
ORDERS = st.integers(1, 2)


@given(
    data=st.data(),
    spec=GRIDS,
    kappa=KAPPAS,
    weight=st.floats(1e-2, 1e2, allow_nan=False, allow_infinity=False),
    order=ORDERS,
)
def test_subtraction_of_two_operators_applies_as_difference(
    data, spec, kappa, weight, order
):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    laplace = _laplace_operator(grid, space, kappa)
    mass = _l2_operator(grid, space, weight)

    # A - B goes through make_operator_addsub(self, other, add=false), the "-" side of the
    # ternary + the (add ? 1. : -1.) coefficient that "+" (already tested) never reaches.
    difference = laplace - mass
    assert difference.num_ops == 2

    n = space.num_DoFs
    x = _make_vector(n, _probe_values(data, n, "x"))

    lhs = difference.apply(x)
    rhs = laplace.apply(x)
    rhs.axpy(-1.0, mass.apply(x))

    scale = (kappa + weight) * n + 1.0
    assert _sup_diff(lhs, rhs) <= 1e-9 * scale


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_subtraction_of_a_vector_applies_as_constant_shift(data, spec, kappa, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    n = space.num_DoFs
    # A - vector goes through make_operator_addsub(self, vector, add=false): the returned lincomb
    # holds a ConstantOperator whose apply yields `vector` regardless of the source, subtracted here.
    vector = _make_vector(n, _probe_values(data, n, "vector"))
    shifted = op - vector
    assert shifted.num_ops == 2

    x = _make_vector(n, _probe_values(data, n, "x"))
    lhs = shifted.apply(x)
    rhs = op.apply(x)
    rhs.axpy(-1.0, vector)

    scale = kappa * n + 1.0
    assert _sup_diff(lhs, rhs) <= 1e-9 * scale


@given(spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_jacobian_options_match_a_reported_type_and_reject_an_unknown_one(
    spec, kappa, order
):
    from dune.gdt import ContinuousLagrangeSpace
    from dune.xt.common import DuneError

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    # A MatrixOperator reports exactly one jacobian option, "matrix" (operators/matrix.hh). The
    # no-argument jacobian_options() list variant is deliberately not called here: its binding
    # returns a std::vector<std::string> without the pybind11/stl.h converter registered, so it
    # raises TypeError at the language boundary (a separate binding limitation, not exercised here).
    # the matching branch: jacobian_options(type) returns the dict whose "type" equals `type`
    opts = op.jacobian_options("matrix")
    assert opts["type"] == "matrix"
    # the non-matching branch: an unreported type is rejected (falls through the loop and throws)
    with pytest.raises(DuneError):
        op.jacobian_options("definitely-not-a-reported-jacobian-type")


@given(spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_invert_options_match_a_reported_type_and_reject_an_unknown_one(
    spec, kappa, order
):
    from dune.gdt import ContinuousLagrangeSpace
    from dune.xt.common import DuneError

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    # Every MatrixOperator always reports the base "newton" invert option (operators/matrix.hh
    # appends it to the linear-solver types via OperatorInterface::all_invert_options). As with
    # jacobian_options above, the no-argument invert_options() list variant is skipped: its
    # std::vector<std::string> return has no registered Python converter in this build.
    opts = op.invert_options("newton")
    assert opts["type"] == "newton"
    with pytest.raises(DuneError):
        op.invert_options("definitely-not-a-reported-invert-type")


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_jacobian_of_operator_plus_vector_ignores_the_constant_summand(
    data, spec, kappa, order
):
    """A + vector is a lincomb of the matrix operator and a ConstantOperator.

    Its jacobian delegates to each summand: the matrix operator adds its matrix, while the
    ConstantOperator (reporting jacobian option "zero") takes the ``type == "zero"`` early return
    and contributes nothing. Hence the assembled jacobian equals the matrix of A alone.
    """
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    n = space.num_DoFs
    vector = _make_vector(n, _probe_values(data, n, "vector"))
    lincomb = op + vector
    assert lincomb.num_ops == 2

    jacobian_op = _empty_matrix_operator(grid, space)
    source = _make_vector(n, _probe_values(data, n, "source"))
    # "lincomb" is the option jacobian_options() reports for a linear combination; it drives each
    # summand with its own reported option ("matrix" for A, "zero" for the ConstantOperator).
    assert lincomb.jacobian(source, jacobian_op, "lincomb") is None

    x = _make_vector(n, _probe_values(data, n, "x"))
    scale = kappa * n + 1.0
    assert _sup_diff(jacobian_op.apply(x), op.apply(x)) <= 1e-9 * scale


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
