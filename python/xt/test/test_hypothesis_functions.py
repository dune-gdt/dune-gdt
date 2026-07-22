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
"""Property tests for the bound XT::Functions interfaces (WP7, #348 / #341): evaluate generic and
expression functions -- and their sums, differences and products -- against the analytic
`Polynomial` / `FourierSum` references from dune.xt.test.hypothesis_strategies.

Which interface files this reaches, given the rule "no new bindings" (#341):

* `dune/xt/functions/interfaces/function.hh` -- `FunctionInterface::evaluate` (single point *and*
  the list-of-points overload) plus the `dim_domain` / `dim_range` / `name` / `static_id`
  properties, bound in python/xt/dune/xt/functions/function-interface.hh, are driven directly here.
* `dune/xt/functions/base/combined.hh` -- the `__add__` / `__sub__` / `__mul__` operators bind
  `SumFunction` / `DifferenceFunction` / `ProductFunction` (see bind_combined_Function /
  addbind_FunctionInterface_combined_op); evaluating the combined result exercises this file.
* `dune/xt/functions/interfaces/element-functions.hh` -- reached only through a grid walk, so the
  visualization smoke test wraps a function in a `GridFunction` and calls `visualize`, which
  localizes it on every element (via function-as-grid-function.hh's ElementFunction wrapper).

`element-flux-functions.hh` (the numerically strongest 5%-coverage candidate named in #348) is
deliberately *not* covered here: no binding exposes a flux function to Python, so lifting it would
require either a new binding (forbidden in this work package, rule 6 of #341) or a native C++ test
(rule 5) -- neither is a "drive the existing bindings with Hypothesis" task.
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    finite_floats,
    fourier_sums,
    has_generic_function,
    polynomials,
)

functions = pytest.importorskip("dune.xt.functions")

HAS_EXPRESSION = hasattr(functions, "ExpressionFunction")
HAS_GENERIC = has_generic_function()

# 1d/2d/3d are the domain dimensions the function-interface bindings instantiate
# (function-interface-1d/2d/3d.cc); every combination below is therefore reachable in any build.
DIMS = (1, 2, 3)

# evaluate() vs. the numpy reference is an exact-arithmetic comparison up to the parser's/callback's
# floating-point round-off; keep evaluation points and polynomial orders modest so a scalar product
# of two functions stays comfortably inside double precision and the tolerance below is meaningful.
EVAL_BOUND = 2.0
MAX_ORDER = 2


@st.composite
def evaluation_points(draw, dim, min_points=1, max_points=8, bound=EVAL_BOUND):
    """An (n, dim) array of bounded finite points -- the argument the list-of-points evaluate takes."""
    num_points = draw(st.integers(min_points, max_points))
    points = [
        [draw(finite_floats(bound)) for _ in range(dim)] for _ in range(num_points)
    ]
    return np.asarray(points, dtype=float)


def evaluate_single(func, point):
    """Scalar value of a bound scalar function at one point, via the single-point evaluate overload."""
    return float(np.asarray(func.evaluate(list(point))).ravel()[0])


def evaluate_list(func, points):
    """Scalar values at many points, via the (rC == 1) list-of-points evaluate overload -> (n,) array."""
    return np.asarray(func.evaluate(points), dtype=float).reshape(points.shape[0])


@pytest.mark.skipif(
    not HAS_EXPRESSION, reason="no ExpressionFunction binding available"
)
@pytest.mark.parametrize("dim", DIMS)
class TestExpressionFunctionEvaluate:
    """FunctionInterface (function.hh) reached through the parsed-expression ExpressionFunction."""

    @settings(deadline=None)
    @given(data=st.data())
    def test_single_point_matches_polynomial(self, dim, data):
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        func = poly.to_function()
        for point in points:
            assert evaluate_single(func, point) == pytest.approx(
                float(poly(point)[0]), rel=1e-9, abs=1e-9
            )

    @settings(deadline=None)
    @given(data=st.data())
    def test_list_of_points_matches_polynomial(self, dim, data):
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        func = poly.to_function()
        actual = evaluate_list(func, points)
        assert actual == pytest.approx(poly(points), rel=1e-9, abs=1e-9)

    @settings(deadline=None)
    @given(data=st.data())
    def test_interface_properties(self, dim, data):
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        func = poly.to_function(name="expr")
        assert func.dim_domain == dim
        assert func.dim_range == 1
        assert func.name == "expr"
        assert isinstance(func.static_id, str)


@pytest.mark.skipif(
    not HAS_GENERIC, reason="no GenericFunction binding available (#320 WP7)"
)
@pytest.mark.parametrize("dim", DIMS)
class TestGenericFunctionEvaluate:
    """FunctionInterface (function.hh) reached through the Python-callback GenericFunction."""

    @settings(deadline=None)
    @given(data=st.data())
    def test_polynomial_single_point(self, dim, data):
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        func = poly.to_generic_function()
        for point in points:
            assert evaluate_single(func, point) == pytest.approx(
                float(poly(point)[0]), rel=1e-9, abs=1e-9
            )

    @settings(deadline=None)
    @given(data=st.data())
    def test_polynomial_list_of_points(self, dim, data):
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        func = poly.to_generic_function()
        actual = evaluate_list(func, points)
        assert actual == pytest.approx(poly(points), rel=1e-9, abs=1e-9)

    @settings(deadline=None)
    @given(data=st.data())
    def test_fourier_sum_matches_reference(self, dim, data):
        fourier = data.draw(fourier_sums(dim=dim))
        points = data.draw(evaluation_points(dim))
        func = fourier.to_generic_function()
        actual = evaluate_list(func, points)
        assert actual == pytest.approx(fourier(points), rel=1e-9, abs=1e-9)

    @settings(deadline=None)
    @given(data=st.data())
    def test_generic_matches_expression(self, dim, data):
        # the callback path (GenericFunction) and the parsed-expression path (ExpressionFunction)
        # compute the same polynomial; agreement between the two is an independent cross-check that
        # neither the C++ expression parser nor the pybind11 callback distorts the evaluation.
        if not HAS_EXPRESSION:
            pytest.skip("no ExpressionFunction binding available")
        poly = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        expr_values = evaluate_list(poly.to_function(), points)
        generic_values = evaluate_list(poly.to_generic_function(), points)
        assert generic_values == pytest.approx(expr_values, rel=1e-9, abs=1e-9)


@pytest.mark.skipif(
    not HAS_EXPRESSION, reason="no ExpressionFunction binding available"
)
@pytest.mark.parametrize("dim", DIMS)
class TestCombinedFunctions:
    """base/combined.hh: sum / difference / product built via the bound operators, then evaluated."""

    @settings(deadline=None)
    @given(data=st.data())
    def test_sum_matches_reference(self, dim, data):
        left = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        right = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        combined = left.to_function(name="l") + right.to_function(name="r")
        actual = evaluate_list(combined, points)
        assert actual == pytest.approx(left(points) + right(points), rel=1e-9, abs=1e-9)

    @settings(deadline=None)
    @given(data=st.data())
    def test_difference_matches_reference(self, dim, data):
        left = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        right = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        combined = left.to_function(name="l") - right.to_function(name="r")
        actual = evaluate_list(combined, points)
        assert actual == pytest.approx(left(points) - right(points), rel=1e-9, abs=1e-9)

    @settings(deadline=None)
    @given(data=st.data())
    def test_product_matches_reference(self, dim, data):
        left = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        right = data.draw(polynomials(dim=dim, max_order=MAX_ORDER))
        points = data.draw(evaluation_points(dim))
        combined = left.to_function(name="l") * right.to_function(name="r")
        actual = evaluate_list(combined, points)
        # a scalar product can amplify round-off relative to the operands, so compare with a
        # slightly looser relative tolerance than the linear sum/difference above.
        assert actual == pytest.approx(left(points) * right(points), rel=1e-7, abs=1e-9)


def _make_cube_grid(dim):
    from dune.xt.grid import Cube, Dim, make_cube_grid

    return make_cube_grid(
        Dim(dim),
        Cube(),
        lower_left=[0.0] * dim,
        upper_right=[1.0] * dim,
        num_elements=[2] * dim,
    )


@pytest.mark.skipif(
    not HAS_EXPRESSION, reason="no ExpressionFunction binding available"
)
@pytest.mark.parametrize("dim", (2, 3))
def test_visualize_localizes_on_every_element(dim, tmp_path):
    """Wrapping a function as a GridFunction and visualizing it walks the grid and localizes it on
    each element, which is the only Python-reachable path into element-functions.hh (the ElementFunction
    interface) -- FunctionInterface itself binds no local_function accessor."""
    grid = pytest.importorskip("dune.xt.grid")
    if not hasattr(grid, "make_cube_grid") or not hasattr(functions, "GridFunction"):
        pytest.skip("no GridFunction / make_cube_grid binding available")

    from dune.xt.grid import Dim

    provider = _make_cube_grid(dim)
    func = functions.ExpressionFunction(
        dim_domain=Dim(dim),
        variable="x",
        order=2,
        expression="x[0]*x[0]",
        name="quadratic",
    )
    grid_function = functions.GridFunction(provider, func)
    prefix = str(tmp_path / f"visualized_{dim}d")
    grid_function.visualize(provider, prefix, False)
    assert any(tmp_path.iterdir()), "visualize wrote no output file"


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
