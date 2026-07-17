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
import math
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


# LA container classes are named `to_camel_case(backend_name + "_" + kind)`, with "Complex"
# inserted right after the backend for std::complex<double>-valued containers (WP5, #320), e.g.
# CommonVector (double) vs. CommonComplexVector (complex), see
# python/xt/dune/xt/la/container.bindings.hh. This keeps field-type detection to a name check
# instead of hard-coding which classes are complex, so it keeps working as more are bound.
LA_FIELDS = ("double", "complex")


def container_field(cls):
    """The LA field ("double" or "complex") a dune.xt.la container class holds."""
    return "complex" if "Complex" in cls.__name__ else "double"


def discover_vector_types(module=None, field="double"):
    """LA vector container classes exposed by dune.xt.la (Common/Istl/Eigen) for the given field.

    field is one of LA_FIELDS ("double", the default, or "complex"); pass field=None for both.
    """
    types = _discover_container_types("Vector", module)
    if field is None:
        return types
    return tuple(t for t in types if container_field(t) == field)


def discover_matrix_types(module=None, field="double"):
    """LA matrix container classes exposed by dune.xt.la (dense/sparse Common, Istl, Eigen).

    field is one of LA_FIELDS ("double", the default, or "complex"); pass field=None for both.
    """
    types = _discover_container_types("Matrix", module)
    if field is None:
        return types
    return tuple(t for t in types if container_field(t) == field)


def _discover_solver_machinery_types(name_suffix, module=None):
    """Matrix classes for which dune.xt.la exposes a `<ClassName><name_suffix>` binding.

    Backs discover_eigen_solver_types / discover_matrix_inverter_types / ...: the eigen-solver,
    generalized eigen-solver and matrix-inverter bindings (WP5, #320) are not added for every
    bound matrix type (only those with matching C++ .tpl test coverage, see the WP5 PR
    description), so this discovers exactly the subset a given build actually bound, the same way
    discover_vector_types/discover_matrix_types do for the containers themselves.
    """
    la = module if module is not None else _import_optional("dune.xt.la")
    if la is None:
        return ()
    result = []
    for matrix_cls in discover_matrix_types(la, field=None):
        if getattr(la, matrix_cls.__name__ + name_suffix, None) is not None:
            result.append(matrix_cls)
    return tuple(result)


def discover_eigen_solver_types(module=None):
    """Matrix classes for which dune.xt.la binds an EigenSolver (dune/xt/la/eigen-solver.hh)."""
    return _discover_solver_machinery_types("EigenSolver", module)


def discover_generalized_eigen_solver_types(module=None):
    """Matrix classes with a bound GeneralizedEigenSolver (dune/xt/la/generalized-eigen-solver.hh)."""
    return _discover_solver_machinery_types("GeneralizedEigenSolver", module)


def discover_matrix_inverter_types(module=None):
    """Matrix classes for which dune.xt.la binds a MatrixInverter (dune/xt/la/matrix-inverter.hh)."""
    return _discover_solver_machinery_types("MatrixInverter", module)


def dense_sparsity_pattern(rows, cols):
    """A SparsityPatternDefault with every (i, j) entry present.

    Every bound LA matrix class -- dense or sparse -- accepts a (rows, cols, SparsityPatternDefault)
    constructor (see internal::addbind_Matrix<T, sparse> in
    python/xt/dune/xt/la/container/matrix-interface.hh), unlike the (rows, cols, value) constructor,
    which is dense-only. This gives property tests one matrix-construction path that works uniformly
    across backends, needed by the WP5 (#320) complex-container and eigen-solver property suites.
    """
    from dune.xt.la import SparsityPatternDefault

    pattern = SparsityPatternDefault(rows)
    for ii in range(rows):
        for jj in range(cols):
            pattern.insert(ii, jj)
    pattern.sort()
    return pattern


# Every GridProvider* the wheel exposes, at the (dim, element, impl) granularity -- so that two
# implementations sharing a (dim, element) slice (e.g. the structured YASP cube and the
# unstructured ALU cube added in #320 WP1) are both drawn by the strategies below.
GRID_BINDINGS = discover_grid_bindings()

# The runtime-reachable slice of the compile-time grid lists used by the C++ typed tests, e.g.
# on a full build: (1, "simplex") ONEDGRID, (2|3, "cube") YASPGRID, (2|3, "simplex") ALUGRID.
GRID_COMBINATIONS = discover_grid_combinations()

# ALUGrid implementations whose LeafGridView is non-conforming (both the ALU cube grid and the
# nonconforming ALU simplex grid parse to this impl -- the element disambiguates them). Several
# GridProvider methods (size/centers for 0 < codim < dim, the intersection-index helpers) are
# guarded with requirements_not_met on non-conforming grids, so properties relying on them draw
# only conforming specs (grid_specs(conforming_only=True)).
_NONCONFORMING_IMPLS = frozenset({"alunonconformgrid"})


def _impl_is_conforming(impl):
    return impl is None or impl not in _NONCONFORMING_IMPLS


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
    """A runtime description of a make_cube_grid call with known structural expectations.

    `impl` selects the grid implementation within a (dim, element) slice, matching the impl word
    of the discovered GridProvider name ("yaspgrid", "onedgrid", "aluconformgrid",
    "alunonconformgrid"). It defaults to None, i.e. the structured/conforming make_cube_grid default
    (YASP cube, conforming ALU simplex, ONEDGRID) -- so a GridSpec built without an impl keeps the
    pre-#320 behaviour. A non-conforming impl is reached via the Nonconforming() factory selector.
    """

    dim: int
    element: str  # "cube" or "simplex"
    lower_left: tuple
    upper_right: tuple
    num_elements: tuple
    impl: str = None

    @property
    def conforming(self):
        """Whether the grid's LeafGridView is conforming (governs which methods are available)."""
        return _impl_is_conforming(self.impl)

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
        elem = {"cube": Cube, "simplex": Simplex}[self.element]()
        if not self.conforming:
            # the unstructured ALU cube/simplex grids share the (dim, element) overload signature
            # with the structured/conforming defaults; Nonconforming() disambiguates them
            from dune.xt.grid import Nonconforming

            return make_cube_grid(Dim(self.dim), elem, Nonconforming(), **kwargs)
        return make_cube_grid(Dim(self.dim), elem, **kwargs)

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


def _sampled_grid_bindings(dims, elements, conforming_only):
    """The (dim, element, impl) triples to draw from, filtered to the caller's needs.

    Drawn from GRID_BINDINGS (all discovered implementations), so both YASP and ALU cube grids --
    and both conforming and nonconforming ALU simplex grids -- are exercised, not just one default
    per (dim, element).
    """
    return [
        (b.dim, b.element, b.impl)
        for b in GRID_BINDINGS
        if b.dim in dims
        and b.element in elements
        and (not conforming_only or _impl_is_conforming(b.impl))
    ]


@st.composite
def grid_specs(
    draw,
    dims=(1, 2, 3),
    elements=("cube", "simplex"),
    max_elements_per_dim=4,
    unit_box=False,
    conforming_only=False,
):
    """Draw a GridSpec for one of the binding-instantiated grid types.

    max_elements_per_dim keeps 3d grids small (a 4^3 cube box already yields 384 simplices);
    hypothesis shrinks towards 1 element per axis. conforming_only restricts the draw to grids
    with a conforming LeafGridView, for properties that use methods the bindings do not implement
    on non-conforming grids (the codim-1 intersection-index helpers).
    """
    dim, element, impl = draw(
        st.sampled_from(_sampled_grid_bindings(dims, elements, conforming_only))
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
        impl=impl,
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

    def to_generic_function(self, name="p"):
        """The same polynomial, evaluated by a Python callable instead of the parsed C++ string.

        Gives every property that exercises `to_function()` a second, independent code path
        (a pybind11 callback into this exact `__call__`, vs. the C++ expression-string parser)
        computing identical math -- the "generic-function variant" #320 WP7 asks the interpolation
        /operator property tests to gain, and unlike `to_function()` needs no subnormal-coefficient
        flushing since there is no float-literal parser involved.
        """
        from dune.xt.functions import GenericFunction
        from dune.xt.grid import Dim

        def evaluate(x, _mu):
            return [float(self(np.asarray(x))[0])]

        return GenericFunction(
            dim_domain=Dim(self.dim),
            dim_range=Dim(1),
            order=self.order,
            evaluate=evaluate,
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


def cg_dofs_per_axis(order, num_elements):
    """Tensor-product Lagrange node count per axis for a scalar CG space on a structured cube grid."""
    return [order * n + 1 for n in num_elements]


def cg_scalar_dof_count(order, num_elements):
    """Expected `num_DoFs` of a scalar `ContinuousLagrangeSpace` on a structured cube `GridSpec`."""
    return int(np.prod(cg_dofs_per_axis(order, num_elements)))


def cg_vector_dof_count(order, num_elements, dim_range):
    """Expected `num_DoFs` of a vector-valued CG space (`dim_range=Dim(dim_range)`) on a cube grid.

    The mapper attaches `dim_range` copies of every scalar Lagrange DoF to the same (sub)entity (the
    `ContinuousMapper` is generic in the local finite element family and does not special-case `r`), so
    the global count is exactly `dim_range` times the scalar space's count -- the vector-CG factory
    overloads added for WP4 (#320) generalize the scalar-only DoF-count property below to this formula.
    """
    return dim_range * cg_scalar_dof_count(order, num_elements)


# --- random Python-callable coefficients with a known exact integral (#320 WP7) -------------
#
# GenericFunction lets hypothesis generate the coefficient *function* itself (not just an
# expression string), including adversarial local features a polynomial or a hand-written
# expression would not think to try. FourierSum keeps the exact integral trivial (every nonzero
# frequency mode integrates to 0 over the unit box) so exactness properties (e.g. "the mean value
# of the interpolant matches the mean value of the interpolated function") can be checked without
# a symbolic integrator, the same way `Polynomial.linear_combination` avoids one for linearity.


# eq=False: see the comment on Polynomial above; the same identity-semantics argument applies here.
@dataclass(frozen=True, eq=False)
class FourierSum:
    """A finite sum of axis-aligned cosine modes: constant + sum_k amplitude_k * cos(2*pi*<freq_k, x> + phase_k).

    Every mode has an integer, non-zero frequency vector, and
    integral_0^1 cos(2*pi*n*t + phase) dt == 0 for any nonzero integer n and any phase, so every
    mode integrates to exactly 0 over the unit box regardless of its amplitude or phase -- only
    the constant term contributes to `exact_integral`.
    """

    dim: int
    constant: float
    modes: tuple  # tuple of (amplitude: float, freq: tuple[int, ...], phase: float)

    def __call__(self, points):
        points = np.atleast_2d(np.asarray(points, dtype=float))
        values = np.full(points.shape[0], self.constant, dtype=float)
        for amplitude, freq, phase in self.modes:
            angle = 2 * np.pi * (points * np.asarray(freq)).sum(axis=1) + phase
            values += amplitude * np.cos(angle)
        return values

    @property
    def exact_integral(self):
        """The exact integral of self over the unit box [0, 1]^dim (see the class docstring)."""
        return self.constant

    def to_generic_function(self, name="fourier_sum"):
        from dune.xt.functions import GenericFunction
        from dune.xt.grid import Dim

        def evaluate(x, _mu):
            return [float(self(np.asarray(x))[0])]

        return GenericFunction(
            dim_domain=Dim(self.dim),
            dim_range=Dim(1),
            # cos(.) is smooth; any order the caller's quadrature needs is exact, 2 is a
            # deliberately-low-but-safe default (matches the polynomial degree of a truncated
            # Taylor expansion most quadrature rules used in these tests are already exact for).
            order=2,
            evaluate=evaluate,
            name=name,
        )


@st.composite
def fourier_sums(draw, dim, max_modes=3, max_frequency=4, amplitude_bound=10.0):
    """Random finite cosine sums (see FourierSum) with a known-exact integral over the unit box."""
    constant = draw(finite_floats(amplitude_bound))
    num_modes = draw(st.integers(0, max_modes))
    modes = []
    for _ in range(num_modes):
        amplitude = draw(finite_floats(amplitude_bound))
        freq = tuple(
            draw(st.integers(-max_frequency, max_frequency)) for _ in range(dim)
        )
        if all(f == 0 for f in freq):
            continue  # a zero frequency mode would silently inflate the constant term
        phase = draw(st.floats(0.0, 2 * math.pi, allow_nan=False, allow_infinity=False))
        modes.append((amplitude, freq, phase))
    return FourierSum(dim=dim, constant=constant, modes=tuple(modes))


# --- unstructured (UG) grids: mixed-element and prism meshes --------------------------------
#
# UGGrid is the only bound grid manager that can hold a mesh with more than one geometry type (or
# a prism); make_mixed_grid / make_prism_grid (python/xt/dune/xt/grid/gridprovider/unstructured.cc)
# mirror the C++ fixtures in dune/gdt/test/spaces/base.hh. Their topology is fixed, so the per
# geometry-type element counts are exact and pin the DG mapper on mixed grids -- something the
# structured (single geometry type) grid_specs above cannot reach (WP3 of #320).


def has_generic_function(module=None):
    """Whether the build binds GenericFunction, i.e. Python callables as C++ functions (#320 WP7).

    Pass an explicit module (e.g. a stub) to test the detection without the compiled wheel.
    """
    functions = module if module is not None else _import_optional("dune.xt.functions")
    return functions is not None and hasattr(functions, "GenericFunction")


def has_uggrid(module=None):
    """Whether the build binds UGGrid, i.e. make_mixed_grid / make_prism_grid are available.

    Pass an explicit module (e.g. a stub) to test the detection without the compiled wheel.
    """
    return any(b.impl == "uggrid" for b in discover_grid_bindings(module))


# (dim, number of corners) -> geometry type, covering every element the bound grids can carry. A
# 1d element is both a simplex and a cube; it is reported as "cube" (Q_k == P_k in 1d anyway).
_GEOMETRY_BY_CORNERS = {
    (1, 2): "cube",
    (2, 3): "simplex",
    (2, 4): "cube",
    (3, 4): "simplex",
    (3, 5): "pyramid",
    (3, 6): "prism",
    (3, 8): "cube",
}


def classify_element(dim, num_corners):
    """Name the geometry type of an element from its dimension and corner count."""
    return _GEOMETRY_BY_CORNERS.get((dim, num_corners), "unknown")


def element_geometry_counts(grid):
    """Count leaf elements per geometry type by walking the grid (exact on mixed grids)."""
    dim = grid.dimension
    counts = {}

    def visit(element):
        geometry_type = classify_element(dim, element.corners.shape[0])
        counts[geometry_type] = counts.get(geometry_type, 0) + 1

    grid.apply_on_each_element(visit)
    return counts


def dg_dofs_per_element(geometry_type, order, dim):
    """Local (discontinuous) Lagrange DoF count for a single element of the given geometry.

    These are the per-geometry blocks that a DG space assembles element by element, so on a mixed
    grid the global DoF count is the sum over elements of this quantity (see
    dg_dof_count_on_mixed_grid).
    """
    if geometry_type == "cube":
        return (order + 1) ** dim  # Q_k, tensor-product Lagrange
    if geometry_type == "simplex":
        return math.comb(order + dim, dim)  # P_k on a d-simplex
    if geometry_type == "prism":
        # a prism is (2-simplex) x line, so its Lagrange space is P_k(triangle) (x) P_k(line)
        return math.comb(order + 2, 2) * (order + 1)
    raise NotImplementedError(
        f"DG DoF count for geometry type {geometry_type!r} is not encoded"
    )


def dg_dof_count_on_mixed_grid(grid, order):
    """Expected number of DoFs of a DiscontinuousLagrangeSpace(grid, order) on any (mixed) grid."""
    dim = grid.dimension
    return sum(
        count * dg_dofs_per_element(geometry_type, order, dim)
        for geometry_type, count in element_geometry_counts(grid).items()
    )


# Topology of the unstructured fixtures before any refinement, as dim -> {geometry type: count};
# mirrors the element insertions in make_mixed_grid / make_prism_grid.
MIXED_GRID_ELEMENTS = {
    2: {"cube": 2, "simplex": 2},
    3: {"cube": 2, "simplex": 2},
}
PRISM_GRID_ELEMENTS = {3: {"prism": 1}}


def make_mixed_grid_provider(dim, num_refinements=0):
    """A UG grid with mixed cube/simplex elements (2d or 3d), as a GridProvider."""
    from dune.xt.grid import Dim, make_mixed_grid

    return make_mixed_grid(Dim(dim), num_refinements=num_refinements)


def make_prism_grid_provider(num_refinements=0):
    """A 3d UG grid consisting of prism elements, as a GridProvider."""
    from dune.xt.grid import Dim, make_prism_grid

    return make_prism_grid(Dim(3), num_refinements=num_refinements)
