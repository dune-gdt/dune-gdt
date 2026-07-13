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

import importlib
import re
from dataclasses import dataclass, field
from itertools import product as _cartesian

import numpy as np
from hypothesis import strategies as st

# --- binding discovery ------------------------------------------------------------------
#
# Rather than hard-coding which grid/container combinations the wheel was built with, we probe
# the compiled modules for the classes they actually expose. A binding adds an instantiation ->
# the class appears in dune.xt.grid / dune.xt.la -> the strategies below pick it up with no edit
# here; a build without an optional dependency (Eigen, ALUGrid, ISTL, ...) simply omits those
# classes and the dependent tests skip. This is the WP0 plumbing that #320's later work packages
# rely on to have their new combinations property-tested automatically.


def _import_optional(module_name):
    """Import a compiled binding module, returning None if the wheel does not provide it."""
    try:
        return importlib.import_module(module_name)
    except ImportError:
        return None


# GridProvider bindings are named `to_camel_case("grid_provider_" + grid_name)` (see
# python/xt/dune/xt/grid/gridprovider.hh), where grid_name is "<dim>d_<element>_<impl>grid"
# (python/xt/dune/xt/grid/grids.bindings.hh), e.g. GridProvider2dCubeYaspgrid,
# GridProvider1dSimplexOnedgrid, GridProvider2dSimplexAluconformgrid. to_camel_case only
# upper-cases the first letter of each underscore-separated word, so the element is a single
# capitalised word and the implementation follows as one more capitalised word.
_GRID_PROVIDER_RE = re.compile(r"^GridProvider(\d+)d([A-Z][a-z]+)([A-Z][A-Za-z0-9]*)$")


@dataclass(frozen=True)
class GridBinding:
    """One compiled `GridProvider*` class, parsed back into its structural coordinates."""

    dim: int
    element: str  # "cube", "simplex", ... (lower case, as in the grid_name)
    impl: str  # grid implementation, e.g. "yaspgrid", "onedgrid", "aluconformgrid"
    class_name: str

    @property
    def grid_name(self):
        return f"{self.dim}d_{self.element}_{self.impl}"


def _parse_grid_provider_name(name):
    """Return (dim, element, impl) for a GridProvider* class name, or None if it is not one."""
    match = _GRID_PROVIDER_RE.match(name)
    if match is None:
        return None
    return int(match.group(1)), match.group(2).lower(), match.group(3).lower()


def discover_grid_bindings(module=None):
    """All GridProvider* classes the dune.xt.grid binding exposes, as GridBinding descriptors.

    Pass an explicit module (e.g. a stub) to test the parsing without the compiled wheel.
    """
    grid = module if module is not None else _import_optional("dune.xt.grid")
    if grid is None:
        return ()
    bindings = []
    for name in dir(grid):
        parsed = _parse_grid_provider_name(name)
        if parsed is None or not isinstance(getattr(grid, name), type):
            continue
        dim, element, impl = parsed
        bindings.append(
            GridBinding(dim=dim, element=element, impl=impl, class_name=name)
        )
    return tuple(sorted(bindings, key=lambda b: (b.dim, b.element, b.impl)))


def discover_grid_combinations(module=None):
    """The de-duplicated (dim, element) grid slice the bindings make reachable at runtime."""
    return tuple(sorted({(b.dim, b.element) for b in discover_grid_bindings(module)}))


def _discover_container_types(suffix, module=None):
    la = module if module is not None else _import_optional("dune.xt.la")
    if la is None:
        return ()
    result = []
    for name in sorted(dir(la)):
        # `CommonVectorSizeT` is an integer index container, not a floating point payload, and
        # ends in "SizeT" rather than the suffix -- so it is naturally excluded from vectors.
        if name == suffix or not name.endswith(suffix):
            continue
        obj = getattr(la, name)
        if isinstance(obj, type):
            result.append(obj)
    return tuple(result)


def discover_vector_types(module=None):
    """Double-valued LA vector container classes exposed by dune.xt.la (Common/Istl/Eigen)."""
    return _discover_container_types("Vector", module)


def discover_matrix_types(module=None):
    """LA matrix container classes exposed by dune.xt.la (dense Common, sparse Istl/Eigen)."""
    return _discover_container_types("Matrix", module)


# The runtime-reachable slice of the compile-time grid lists used by the C++ typed tests, e.g.
# on a full build: (1, "simplex") ONEDGRID, (2|3, "cube") YASPGRID, (2|3, "simplex") ALUGRID.
GRID_COMBINATIONS = discover_grid_combinations()


def has_grid(dim=None, element=None):
    """Whether the current build binds a grid with the given dimension and/or element type."""
    return any(
        (dim is None or d == dim) and (element is None or e == element)
        for (d, e) in GRID_COMBINATIONS
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

        kwargs = {
            "lower_left": list(self.lower_left),
            "upper_right": list(self.upper_right),
            "num_elements": list(self.num_elements),
        }
        if self.dim == 1 and self.element == "simplex":
            # ONEDGRID is bound through the dimension-only make_cube_grid overload (no element
            # type); the structured 1d YaspGrid (WP2, #320) instead uses the (Dim, Cube) overload
            # like its 2d/3d siblings, handled by the general branch below.
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
    upper = tuple(ll + e for ll, e in zip(lower, extents, strict=True))
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


# eq=False: with the (unhashable) coefficients dict excluded from comparison, the generated
# __eq__/__hash__ would consider two polynomials equal iff they share the dimension; identity
# semantics are safer for objects hypothesis puts into failure reports and caches
@dataclass(frozen=True, eq=False)
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
