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
"""Operator-interface algebra: apply, linearity and linear combinations of assembled MatrixOperators.

Scenario 5 (test_hypothesis_operators.py) drives the *assembly* stack and inspects only ``op.matrix``. This
module instead exercises the *operator interface* itself (dune/gdt/operators/interfaces.hh) and the linear
combination operators it returns (dune/gdt/operators/lincomb.hh), which the C++ suite covers only indirectly:

  * ``op.apply(x)`` reproduces the plain matrix-vector product ``op.matrix @ x`` (the matrix-based operator is,
    by definition, the linear map represented by its assembled matrix);
  * ``apply`` is linear: ``op.apply(a*x + y) == a*op.apply(x) + op.apply(y)`` exactly (up to accumulation);
  * the linear combination ``a*L + b*M`` (an operator returned by ``L*a + M*b``) applies as
    ``a*L.apply(x) + b*M.apply(x)`` and reports ``num_ops == 2``; negation ``-L`` flips the sign of apply;
  * every assembled matrix operator reports ``linear == True``;
  * subtraction ``A - B`` / ``A - vector`` applies as the corresponding difference (the ``"-"`` side of
    ``make_operator_addsub``), ``jacobian_options(type)`` / ``invert_options(type)`` match a reported type
    and reject an unknown one, and the jacobian of ``A + vector`` ignores its ConstantOperator summand
    (the ``type == "zero"`` early return).

All inputs (grid geometry, order, the two scalar coefficients and the probe vectors) are hypothesis-generated;
only the five binding-instantiated grid types are fixed.
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


def _matvec(matrix, vec):
    from dune.xt.la import IstlVector

    result = IstlVector(len(vec), 0.0)
    matrix.mv(vec, result)
    return result


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
SCALARS = st.floats(
    -10.0, 10.0, allow_nan=False, allow_infinity=False, allow_subnormal=False
)


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_apply_equals_matrix_vector_product(data, spec, kappa, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    x = _make_vector(space.num_DoFs, _probe_values(data, space.num_DoFs, "x"))
    applied = op.apply(x)
    expected = _matvec(op.matrix, x)
    assert op.linear is True
    scale = kappa * space.num_DoFs + 1.0
    assert _sup_diff(applied, expected) <= 1e-10 * scale


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS, scalar=SCALARS)
def test_apply_is_linear(data, spec, kappa, order, scalar):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    n = space.num_DoFs
    x_vals = _probe_values(data, n, "x")
    y_vals = _probe_values(data, n, "y")
    x = _make_vector(n, x_vals)
    y = _make_vector(n, y_vals)
    combo = _make_vector(n, scalar * x_vals + y_vals)

    lhs = op.apply(combo)
    rhs = op.apply(x)
    rhs.scal(scalar)
    rhs.axpy(1.0, op.apply(y))

    scale = kappa * n * (abs(scalar) + 1.0) + 1.0
    assert _sup_diff(lhs, rhs) <= 1e-9 * scale


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_apply2_binding_returns_none_but_executes(data, spec, kappa, order):
    """The C++ apply2 is exercised for coverage; its pybind11 wrapper currently returns void (None).

    This mirrors the documented-contract style of test_hypothesis_spaces.py: the call must still run without
    error on every generated example, and if the binding is ever changed to return the value this assertion is
    the single place that records the current behaviour.
    """
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    x = _make_vector(space.num_DoFs, _probe_values(data, space.num_DoFs, "x"))
    y = _make_vector(space.num_DoFs, _probe_values(data, space.num_DoFs, "y"))
    assert op.apply2(y, x) is None


@given(
    data=st.data(),
    spec=GRIDS,
    kappa=KAPPAS,
    weight=st.floats(1e-2, 1e2, allow_nan=False, allow_infinity=False),
    order=ORDERS,
    a=SCALARS,
    b=SCALARS,
)
def test_linear_combination_applies_componentwise(
    data, spec, kappa, weight, order, a, b
):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    laplace = _laplace_operator(grid, space, kappa)
    mass = _l2_operator(grid, space, weight)

    # a*L + b*M is returned as a lincomb operator (dune/gdt/operators/lincomb.hh)
    combined = laplace * a + mass * b
    assert combined.num_ops == 2

    n = space.num_DoFs
    x = _make_vector(n, _probe_values(data, n, "x"))

    lhs = combined.apply(x)
    rhs = laplace.apply(x)
    rhs.scal(a)
    mx = mass.apply(x)
    mx.scal(b)
    rhs.axpy(1.0, mx)

    scale = (abs(a) * kappa + abs(b) * weight) * n + 1.0
    assert _sup_diff(lhs, rhs) <= 1e-9 * scale


@given(data=st.data(), spec=GRIDS, kappa=KAPPAS, order=ORDERS)
def test_negation_flips_apply(data, spec, kappa, order):
    from dune.gdt import ContinuousLagrangeSpace

    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    op = _laplace_operator(grid, space, kappa)

    n = space.num_DoFs
    x = _make_vector(n, _probe_values(data, n, "x"))

    negated = (-op).apply(x)
    expected = op.apply(x)
    expected.scal(-1.0)

    scale = kappa * n + 1.0
    assert _sup_diff(negated, expected) <= 1e-10 * scale


# The remaining tests cover the *other* sign of the arithmetic ternaries and the option-selection
# machinery of dune/gdt/operators/interfaces.hh, which the tests above (only +, *, unary -) never
# reach: subtraction A - B / A - vector, jacobian_options/invert_options type match-and-mismatch,
# and the ConstantOperator "zero" jacobian branch reached through a lincomb A + vector.


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
