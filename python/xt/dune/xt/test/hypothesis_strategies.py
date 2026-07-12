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
"""Shared hypothesis strategies for driving the compiled dune-xt/dune-gdt bindings from Python.

The C++ test suite parametrizes over grid types, dimensions and polynomial orders at compile time
(TYPED_TEST_SUITE grid lists and the dxt_code_generation .tpl cross products), which makes every
new parameter combination another translation unit to compile. The bindings already instantiate
one fixed set of grid/space/container combinations; everything below turns the remaining
parametrization (geometry, resolutions, orders, coefficients, DoF data) into *runtime* values
drawn by hypothesis, so a single compiled artifact is exercised with many generated inputs.

This module ships with the dune.xt wheel (like dune.xt.test.grid_types) so both the dune-xt and
the dune-gdt pytest suites can import the same strategies.
"""

from dataclasses import dataclass, field
from itertools import product as _cartesian

import numpy as np
from hypothesis import strategies as st

# The bindings pre-instantiate exactly these grid types (see the GridProvider* binding names):
# 1d ONEDGRID, 2d/3d cube YASPGRID and 2d/3d simplex ALUGRID (conforming). This tuple *is* the
# runtime-reachable slice of the compile-time grid lists used by the C++ typed tests.
GRID_COMBINATIONS = (
    (1, "simplex"),  # GridProvider1dSimplexOnedgrid
    (2, "cube"),  # GridProvider2dCubeYaspgrid
    (2, "simplex"),  # GridProvider2dSimplexAluconformgrid
    (3, "cube"),  # GridProvider3dCubeYaspgrid
    (3, "simplex"),  # GridProvider3dSimplexAluconformgrid
)

# Number of simplices a DGF cube is split into by make_cube_grid per dimension.
SIMPLICES_PER_CUBE = {1: 1, 2: 2, 3: 6}


@dataclass(frozen=True)
class GridSpec:
    """A runtime description of a make_cube_grid call with known structural expectations."""

    dim: int
    element: str  # "cube" or "simplex"
    lower_left: tuple
    upper_right: tuple
    num_elements: tuple

    def make_grid(self):
        from dune.xt.grid import Cube, Dim, Simplex, make_cube_grid

        kwargs = dict(
            lower_left=list(self.lower_left),
            upper_right=list(self.upper_right),
            num_elements=list(self.num_elements),
        )
        if self.dim == 1:  # the 1d overload (ONEDGRID) takes no element type
            return make_cube_grid(Dim(1), **kwargs)
        elem = {"cube": Cube, "simplex": Simplex}[self.element]
        return make_cube_grid(Dim(self.dim), elem(), **kwargs)

    @property
    def num_cubes(self):
        return int(np.prod(self.num_elements))

    @property
    def expected_num_elements(self):
        if self.element == "cube":
            return self.num_cubes
        return SIMPLICES_PER_CUBE[self.dim] * self.num_cubes

    @property
    def expected_num_vertices(self):
        # Vertices of the structured cube skeleton; splitting cubes into simplices adds no
        # vertices (Freudenthal/Kuhn triangulation), so this holds for both element types.
        return int(np.prod([n + 1 for n in self.num_elements]))


@st.composite
def bounding_boxes(draw, dim, coordinate_range=100.0, min_extent=0.1, max_extent=100.0):
    """Axis-aligned boxes with strictly positive, non-degenerate extent per axis.

    Extents below min_extent would produce nearly-degenerate cells whose floating point
    geometry drowns every property in round-off; that is a conditioning concern, not a
    correctness concern, so tiny boxes are simply not generated.
    """
    lower = draw(
        st.tuples(
            *[
                st.floats(
                    -coordinate_range,
                    coordinate_range,
                    allow_nan=False,
                    allow_infinity=False,
                )
                for _ in range(dim)
            ]
        )
    )
    extents = draw(
        st.tuples(
            *[
                st.floats(min_extent, max_extent, allow_nan=False, allow_infinity=False)
                for _ in range(dim)
            ]
        )
    )
    upper = tuple(ll + e for ll, e in zip(lower, extents))
    return lower, upper


@st.composite
def grid_specs(
    draw,
    dims=(1, 2, 3),
    elements=("cube", "simplex"),
    max_elements_per_dim=4,
    unit_box=False,
):
    """Draw a GridSpec for one of the binding-instantiated grid types.

    max_elements_per_dim keeps 3d grids small (a 4^3 cube box already yields 384 simplices);
    hypothesis shrinks towards 1 element per axis.
    """
    dim, element = draw(
        st.sampled_from(
            [(d, e) for (d, e) in GRID_COMBINATIONS if d in dims and e in elements]
        )
    )
    if unit_box:
        lower, upper = (0.0,) * dim, (1.0,) * dim
    else:
        lower, upper = draw(bounding_boxes(dim))
    num_elements = draw(
        st.tuples(*[st.integers(1, max_elements_per_dim) for _ in range(dim)])
    )
    return GridSpec(
        dim=dim,
        element=element,
        lower_left=lower,
        upper_right=upper,
        num_elements=num_elements,
    )


def finite_floats(bound=1e6):
    """Well-behaved test scalars: finite, bounded, no signed-zero surprises in comparisons."""
    return st.floats(
        min_value=-bound, max_value=bound, allow_nan=False, allow_infinity=False
    )


@st.composite
def vector_data(draw, min_size=1, max_size=64, bound=1e6):
    """Plain python lists of bounded finite doubles, the payload for LA container tests."""
    return draw(st.lists(finite_floats(bound), min_size=min_size, max_size=max_size))


def _monomial_expression(alpha):
    factors = []
    for axis, power in enumerate(alpha):
        factors.extend([f"x[{axis}]"] * power)
    return "*".join(factors) if factors else "1.0"


@dataclass(frozen=True)
class Polynomial:
    """A multivariate polynomial with an exact dune ExpressionFunction representation.

    Coefficients are kept as an explicit {multi-index: value} map so linear combinations of
    polynomials (needed to state linearity properties of C++ operators) stay exact: the
    combination is computed on the coefficients in Python and rendered to a new expression
    string, rather than relying on any floating point identity of parsed expressions.
    """

    dim: int
    coefficients: dict = field(compare=False)  # {(a_1, ..., a_dim): float}

    @property
    def order(self):
        return max((sum(alpha) for alpha in self.coefficients), default=0)

    def expression(self):
        terms = [
            f"({coeff!r})*{_monomial_expression(alpha)}"
            for alpha, coeff in sorted(self.coefficients.items())
        ]
        return " + ".join(terms) if terms else "0.0"

    def to_function(self, name="p"):
        from dune.xt.functions import ExpressionFunction
        from dune.xt.grid import Dim

        return ExpressionFunction(
            dim_domain=Dim(self.dim),
            variable="x",
            # ExpressionFunction cannot know the polynomial degree of a parsed string; the
            # declared integration order must match the true degree for exactness properties.
            order=self.order,
            expression=self.expression(),
            name=name,
        )

    def __call__(self, points):
        points = np.atleast_2d(np.asarray(points, dtype=float))
        values = np.zeros(points.shape[0])
        for alpha, coeff in self.coefficients.items():
            values += coeff * np.prod(points ** np.asarray(alpha), axis=1)
        return values

    def linear_combination(self, scalar, other):
        """self * scalar + other, computed exactly on the coefficient map.

        Coefficients that fall below DBL_MIN are flushed to 0.0: the C++ expression parser
        rejects subnormal float literals (see the comment in `polynomials`), and products of
        two admissible coefficients can land in that range.
        """
        assert self.dim == other.dim
        coeffs = {alpha: scalar * coeff for alpha, coeff in self.coefficients.items()}
        for alpha, coeff in other.coefficients.items():
            coeffs[alpha] = coeffs.get(alpha, 0.0) + coeff
        dbl_min = np.finfo(np.float64).tiny
        coeffs = {
            alpha: (0.0 if abs(coeff) < dbl_min else coeff)
            for alpha, coeff in coeffs.items()
        }
        return Polynomial(dim=self.dim, coefficients=coeffs)


@st.composite
def polynomials(draw, dim, max_order=3, coefficient_bound=100.0, min_order=0):
    """Random polynomials of total degree <= max_order with bounded coefficients."""
    order = draw(st.integers(min_order, max_order))
    multi_indices = [
        alpha
        for alpha in _cartesian(range(order + 1), repeat=dim)
        if sum(alpha) <= order
    ]
    coefficients = {
        # allow_subnormal=False is load-bearing: the C++ expression parser behind
        # ExpressionFunction rejects float literals below DBL_MIN (e.g. 2.2e-309) with
        # wrong_input_given -- found by hypothesis shrinking on the interpolation tests.
        alpha: draw(
            st.floats(
                -coefficient_bound,
                coefficient_bound,
                allow_nan=False,
                allow_infinity=False,
                allow_subnormal=False,
            )
        )
        for alpha in multi_indices
    }
    return Polynomial(dim=dim, coefficients=coefficients)
