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
"""Grid adaptation drives the spaces' restrict/prolong machinery (dune/gdt/spaces/interface.hh).

The pre_adapt/adapt/post_adapt cycle of the AdaptationHelper is the only route through
SpaceInterface::restrict_to / prolong_onto and through the backup/restore machinery of the
global bases (dune/gdt/spaces/basis/): nothing else in the Python or C++ test suites ever
adapts a grid underneath a space, which left that code (and the mappers'
update_after_adapt) essentially uncovered. The properties here are exactness statements:
refining or coarsening a grid must not change a discrete function that the space can
represent exactly, so the prolonged/restricted DoF vectors are pinned against a fresh
interpolation of the same polynomial on the adapted grid.

The AdaptationHelper is only instantiated for the adaptive grids (OneDGrid and the
conforming ALU simplex grids); YaspGrid is excluded upstream (its PersistentContainer is
known to segfault, see dune/gdt/tools/adaptation-helper.hh).
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    GRID_BINDINGS,
    GridSpec,
    fv_mass,
    has_gdt_bindings,
    polynomials,
)

# The helper binds ONED_1D and the conforming ALU simplex grids (AvailableAdaptiveGridTypes in
# python/gdt/dune/gdt/tools/adaptation-helper.cc); intersect with what this build provides.
_ADAPTIVE_IMPLS = ("onedgrid", "aluconformgrid")
ADAPTIVE_GRIDS = tuple(b for b in GRID_BINDINGS if b.impl in _ADAPTIVE_IMPLS)

pytestmark = pytest.mark.skipif(
    not ADAPTIVE_GRIDS or not has_gdt_bindings("AdaptationHelper"),
    reason="no adaptive grid bindings available in this build",
)


@st.composite
def adaptive_grid_specs(draw, dims=(1, 2, 3), max_elements_per_dim=2):
    """A GridSpec for one of the adaptation-capable grids, on the unit box.

    The unit box keeps the L2 projections behind restrict_to well conditioned; the geometry
    itself is not the property under test here. max_elements_per_dim stays small because each
    example runs several full adaptation cycles on 1-3 appended spaces.
    """
    binding = draw(st.sampled_from([b for b in ADAPTIVE_GRIDS if b.dim in dims]))
    num_elements = draw(
        st.tuples(*[st.integers(1, max_elements_per_dim) for _ in range(binding.dim)])
    )
    return GridSpec(
        dim=binding.dim,
        element=binding.element,
        lower_left=(0.0,) * binding.dim,
        upper_right=(1.0,) * binding.dim,
        num_elements=num_elements,
        impl=binding.impl,
    )


def _make_helper(grid, dim_range):
    """An AdaptationHelper for the given range dimension, tolerant of the old factory.

    The vector-valued factory used to register its dim_range/la_backend keyword names swapped
    (fixed alongside these tests), so fall back to the old positional order if the keyword call
    is rejected -- that keeps the suite runnable against wheels predating the fix.
    """
    from dune.gdt import AdaptationHelper
    from dune.xt.grid import Dim
    from dune.xt.la import Istl

    if dim_range == 1:
        return AdaptationHelper(grid)
    try:
        return AdaptationHelper(grid, dim_range=Dim(dim_range))
    except TypeError:
        pass
    try:
        return AdaptationHelper(grid, Istl(), Dim(dim_range))
    except TypeError:
        # neither the fixed nor the old-swapped signature exists -> no vector-valued helper
        pytest.skip("this build has no vector-valued AdaptationHelper")


def _supports_signed_markers(helper):
    """Whether the markers vector can hold the -1 that requests coarsening.

    Before the fix that accompanies these tests the helper stored size_t markers, so
    coarsening was not expressible from Python at all (and the restrict_to code below it was
    unreachable).
    """
    return "SizeT" not in type(helper.markers).__name__


def _mark(helper, element_indices, dim_range, value):
    """Set the marker of the given elements to value (the markers vector holds r entries per
    element, see the marker_indices FiniteVolumeSpace in the binding)."""
    markers = helper.markers
    for index in element_indices:
        markers[index * dim_range] = value
    helper.mark()


def _refinement_markers(draw, num_elements):
    """A non-empty subset of element indices to refine."""
    return draw(
        st.sets(
            st.integers(0, num_elements - 1), min_size=1, max_size=min(num_elements, 4)
        )
    )


def _scalar_function(poly):
    return poly.to_function()


def _interpolate(grid, function, space, dim_range=1):
    from dune.gdt import default_interpolation
    from dune.xt.functions import GridFunction
    from dune.xt.grid import Dim

    if dim_range == 1:
        return default_interpolation(GridFunction(grid, function), space)
    return default_interpolation(
        GridFunction(grid, function, dim_range=Dim(dim_range)), space
    )


def _adaptation_cycle(helper, marked, dim_range=1, value=1):
    _mark(helper, marked, dim_range, value)
    helper.pre_adapt()
    helper.adapt()
    helper.post_adapt()


@settings(max_examples=10)
@given(spec=adaptive_grid_specs(), data=st.data())
def test_cg_prolongation_preserves_representable_polynomials(spec, data):
    """Refinement must reproduce any function the coarse CG space could already represent:
    SpaceInterface::prolong_onto interpolates the father's data on each new element, which is
    exact for polynomials of degree <= order. Pins prolong_onto (both the isNew and the
    unchanged-element branch) plus the ContinuousMapper/DefaultGlobalBasis update_after_adapt.
    """
    from dune.gdt import ContinuousLagrangeSpace

    # the ContinuousMapper supports order <= 2 on the simplicial grids (see
    # test_hypothesis_spaces._cg_orders)
    order = data.draw(st.integers(1, 2), label="order")
    poly = data.draw(
        polynomials(spec.dim, max_order=order, coefficient_bound=10.0), label="poly"
    )
    grid = spec.make_grid()
    space = ContinuousLagrangeSpace(grid, order=order)
    u_h = _interpolate(grid, _scalar_function(poly), space)

    helper = _make_helper(grid, 1)
    helper.append(space, u_h)
    marked = _refinement_markers(data.draw, grid.size(0))
    _adaptation_cycle(helper, marked)

    fresh = _interpolate(
        grid, _scalar_function(poly), ContinuousLagrangeSpace(grid, order=order)
    )
    scale = max(1.0, np.abs(np.array(fresh.dofs.vector)).max())
    assert np.allclose(
        np.array(u_h.dofs.vector), np.array(fresh.dofs.vector), atol=1e-9 * scale
    )


@settings(max_examples=10)
@given(spec=adaptive_grid_specs(), data=st.data())
def test_dg_prolongation_preserves_representable_polynomials(spec, data):
    """The DG analogue of the CG property above; additionally pins the DiscontinuousMapper's
    update_after_adapt (scalar branch) which recomputes the per-element DoF offsets."""
    from dune.gdt import DiscontinuousLagrangeSpace

    order = data.draw(st.integers(0, 2), label="order")
    poly = data.draw(
        polynomials(spec.dim, max_order=order, coefficient_bound=10.0), label="poly"
    )
    grid = spec.make_grid()
    space = DiscontinuousLagrangeSpace(grid, order=order)
    u_h = _interpolate(grid, _scalar_function(poly), space)

    helper = _make_helper(grid, 1)
    helper.append(space, u_h)
    marked = _refinement_markers(data.draw, grid.size(0))
    _adaptation_cycle(helper, marked)

    fresh = _interpolate(
        grid, _scalar_function(poly), DiscontinuousLagrangeSpace(grid, order=order)
    )
    scale = max(1.0, np.abs(np.array(fresh.dofs.vector)).max())
    assert np.allclose(
        np.array(u_h.dofs.vector), np.array(fresh.dofs.vector), atol=1e-9 * scale
    )


@settings(max_examples=10)
@given(spec=adaptive_grid_specs(), data=st.data())
def test_fv_prolongation_copies_father_values_and_conserves_mass(spec, data):
    """FiniteVolumeSpace overrides prolong_onto with a plain copy from the father element, so
    refinement changes neither the set of DoF values nor the discrete integral (mass)."""
    from dune.gdt import FiniteVolumeSpace

    poly = data.draw(
        polynomials(spec.dim, max_order=1, coefficient_bound=10.0), label="poly"
    )
    grid = spec.make_grid()
    space = FiniteVolumeSpace(grid)
    u_h = _interpolate(grid, _scalar_function(poly), space)
    old_values = np.array(u_h.dofs.vector, copy=True)
    old_mass = fv_mass(u_h, grid)

    helper = _make_helper(grid, 1)
    helper.append(space, u_h)
    marked = _refinement_markers(data.draw, grid.size(0))
    _adaptation_cycle(helper, marked)

    new_values = np.array(u_h.dofs.vector)
    assert space.num_DoFs == grid.size(0) == len(new_values)
    # children copy their father's DoF bit-exactly, unchanged elements keep theirs
    assert set(new_values).issubset(set(old_values))
    assert np.isclose(fv_mass(u_h, grid), old_mass, rtol=1e-12, atol=1e-12)


@pytest.mark.skipif(
    not any(b.dim > 1 for b in ADAPTIVE_GRIDS),
    reason="no multi-dimensional adaptive grid bindings available in this build",
)
@settings(max_examples=10)
@given(spec=adaptive_grid_specs(dims=(2, 3)), data=st.data())
def test_vector_dg_dimwise_mapper_survives_adaptation(spec, data):
    """A vector-valued DG space defaults to the dimension-wise DoF numbering, so adapting it
    runs DiscontinuousMapper::update_after_adapt through its per-range-dimension (else)
    branches -- unreachable from the scalar tests. The exactness property is the same as in
    the scalar DG test, per component."""
    from dune.gdt import DiscontinuousLagrangeSpace
    from dune.xt.functions import ExpressionFunction
    from dune.xt.grid import Dim

    r = spec.dim
    order = data.draw(st.integers(0, 1), label="order")
    components = [
        data.draw(
            polynomials(spec.dim, max_order=order, coefficient_bound=10.0),
            label=f"poly[{i}]",
        )
        for i in range(r)
    ]
    function = ExpressionFunction(
        dim_domain=Dim(spec.dim),
        variable="x",
        expressions=[p.expression() for p in components],
        order=order,
        name="v",
    )
    grid = spec.make_grid()

    def make_space():
        return DiscontinuousLagrangeSpace(grid, order=order, dim_range=Dim(r))

    try:
        space = make_space()
    except TypeError:
        pytest.skip("this build has no vector-valued DG factory")
    assert space.dimwise_global_mapping is True
    u_h = _interpolate(grid, function, space, dim_range=r)

    helper = _make_helper(grid, r)
    helper.append(space, u_h)
    marked = _refinement_markers(data.draw, grid.size(0))
    _adaptation_cycle(helper, marked, dim_range=r)

    fresh = _interpolate(grid, function, make_space(), dim_range=r)
    scale = max(1.0, np.abs(np.array(fresh.dofs.vector)).max())
    assert np.allclose(
        np.array(u_h.dofs.vector), np.array(fresh.dofs.vector), atol=1e-9 * scale
    )


def _vector_l2_form(grid, spec):
    """A BilinearForm with the L2 product integrand for dim-valued functions, tolerant of
    builds predating the (ansatz_range, test_range) factory overloads."""
    import dune.gdt
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        LocalElementProductIntegrand,
    )
    from dune.xt.functions import GridFunction
    from dune.xt.grid import Dim

    d = spec.dim
    if d == 1:
        form = BilinearForm(grid)
        integrand = LocalElementProductIntegrand(GridFunction(grid, 1.0))
    else:
        try:
            form = BilinearForm(grid, ansatz_range=Dim(d), test_range=Dim(d))
        except TypeError:
            # builds predating the (ansatz_range, test_range) factory overloads still bind
            # the class itself under its camel-cased name (verified against those wheels)
            impl = "".join(w.capitalize() for w in spec.impl.split("_"))
            cls = getattr(
                dune.gdt,
                f"BilinearForm{d}dSimplex{impl}Leaf{d}dRange{d}dSource",
                None,
            )
            if cls is None:
                pytest.skip("this build has no vector-valued BilinearForm binding")
            form = cls(grid)
        eye = [[1.0 if ii == jj else 0.0 for jj in range(d)] for ii in range(d)]
        integrand = LocalElementProductIntegrand(GridFunction(grid, eye))
    form += LocalElementIntegralBilinearForm(integrand)
    return form


def _as_grid_function(grid, discrete_function, dim_range):
    from dune.xt.functions import GridFunction
    from dune.xt.grid import Dim

    if dim_range == 1:
        return GridFunction(grid, discrete_function)
    return GridFunction(grid, discrete_function, dim_range=Dim(dim_range))


@settings(max_examples=10)
@given(spec=adaptive_grid_specs(), data=st.data())
def test_rt_prolongation_preserves_the_function(spec, data):
    """RT0 is affine-invariant, so the father's function restricted to a child is again a
    local RT0 function and prolongation (RT basis interpolate/evaluate + backup/restore, the
    bulk of dune/gdt/spaces/basis/raviart-thomas.hh) must reproduce it exactly -- checked via
    the L2 norm, which the bindings expose through BilinearForm.apply2. Broken before the
    accompanying fix: ConstLocalDiscreteFunction held a *copy* of the space, so the stored
    local functions read the pre-adaptation RT FE data."""
    from dune.gdt import DiscreteFunction, RaviartThomasSpace

    grid = spec.make_grid()
    space = RaviartThomasSpace(grid, order=0)
    u_h = DiscreteFunction(space, name="q")
    vector = u_h.dofs.vector
    dof_values = data.draw(
        st.lists(
            st.floats(-10.0, 10.0, allow_nan=False, allow_infinity=False),
            min_size=len(vector),
            max_size=len(vector),
        ),
        label="dofs",
    )
    for ii, value in enumerate(dof_values):
        vector[ii] = value
    l2_before = _vector_l2_form(grid, spec).apply2(
        _as_grid_function(grid, u_h, spec.dim), _as_grid_function(grid, u_h, spec.dim)
    )

    helper = _make_helper(grid, spec.dim)
    helper.append(space, u_h)
    marked = _refinement_markers(data.draw, grid.size(0))
    _adaptation_cycle(helper, marked, dim_range=spec.dim)

    assert space.num_DoFs == len(u_h.dofs.vector)
    l2_after = _vector_l2_form(grid, spec).apply2(
        _as_grid_function(grid, u_h, spec.dim), _as_grid_function(grid, u_h, spec.dim)
    )
    assert np.isclose(l2_after, l2_before, rtol=1e-8, atol=1e-10)


@settings(max_examples=10)
@given(spec=adaptive_grid_specs(), data=st.data())
def test_coarsening_restores_representable_polynomials(spec, data):
    """Coarsening runs SpaceInterface::restrict_to (an element-local L2 projection) resp. the
    FiniteVolumeSpace override (a volume-weighted average); for a function the coarse space
    can represent exactly, both are exact. Refine everything, then coarsen everything, and pin
    the result against a fresh interpolation on whatever grid the coarsening produced."""
    from dune.gdt import (
        ContinuousLagrangeSpace,
        DiscontinuousLagrangeSpace,
        FiniteVolumeSpace,
    )

    kind = data.draw(st.sampled_from(["cg", "dg", "fv"]), label="kind")
    order = {"cg": 1, "dg": data.draw(st.integers(0, 1), label="order"), "fv": 0}[kind]
    # the polynomial must be representable in the *space* for restriction to be exact -- except
    # for FV, whose restriction (volume-weighted averaging of exact cell averages) is exact for
    # any function the fine interpolation integrated exactly
    poly = data.draw(
        polynomials(spec.dim, max_order=1 if kind == "fv" else order),
        label="poly",
    )

    def make_space(grid):
        if kind == "cg":
            return ContinuousLagrangeSpace(grid, order=order)
        if kind == "dg":
            return DiscontinuousLagrangeSpace(grid, order=order)
        return FiniteVolumeSpace(grid)

    grid = spec.make_grid()
    space = make_space(grid)
    u_h = _interpolate(grid, _scalar_function(poly), space)

    helper = _make_helper(grid, 1)
    if not _supports_signed_markers(helper):
        pytest.skip("this build's AdaptationHelper cannot mark for coarsening")
    helper.append(space, u_h)
    # refine everything, so there is something to coarsen ...
    _adaptation_cycle(helper, range(grid.size(0)))
    # ... then coarsen everything
    _adaptation_cycle(helper, range(grid.size(0)), value=-1)

    fresh = _interpolate(grid, _scalar_function(poly), make_space(grid))
    assert space.num_DoFs == len(fresh.dofs.vector)
    scale = max(1.0, np.abs(np.array(fresh.dofs.vector)).max())
    # restriction solves small element-local L2 systems, so allow for a little more noise
    # than the pure-interpolation properties above
    assert np.allclose(
        np.array(u_h.dofs.vector), np.array(fresh.dofs.vector), atol=1e-7 * scale
    )


@settings(max_examples=5)
@given(spec=adaptive_grid_specs())
def test_skeleton_space_prolongation_is_not_implemented(spec):
    """FiniteVolumeSkeletonSpace deliberately throws in prolong_onto (and updates its mapper
    and basis in update_after_adapt on the way there) -- the documented not-implemented
    contract, pinned here so silently wrong prolongations cannot appear instead."""
    from dune.gdt import DiscreteFunction, FiniteVolumeSkeletonSpace
    from dune.xt.common import DuneError

    grid = spec.make_grid()
    space = FiniteVolumeSkeletonSpace(grid)
    u_h = DiscreteFunction(space, name="s")
    helper = _make_helper(grid, 1)
    helper.append(space, u_h)
    _mark(helper, range(grid.size(0)), 1, 1)
    helper.pre_adapt()
    with pytest.raises(DuneError, match="[Nn]ot implemented"):
        helper.adapt()


@settings(max_examples=5)
@given(spec=adaptive_grid_specs())
def test_skeleton_space_restriction_is_not_implemented(spec):
    """The restrict_to counterpart of the property above: refine first (without the skeleton
    space appended, since its prolongation throws), then request coarsening with the skeleton
    space appended -- pre_adapt must hit its not-implemented restrict_to."""
    from dune.gdt import DiscreteFunction, FiniteVolumeSkeletonSpace
    from dune.xt.common import DuneError

    grid = spec.make_grid()
    helper = _make_helper(grid, 1)
    if not _supports_signed_markers(helper):
        pytest.skip("this build's AdaptationHelper cannot mark for coarsening")
    # refine with no space appended, purely to create coarsenable elements
    _adaptation_cycle(helper, range(grid.size(0)))

    space = FiniteVolumeSkeletonSpace(grid)
    u_h = DiscreteFunction(space, name="s")
    helper.append(space, u_h)
    _mark(helper, range(grid.size(0)), 1, -1)
    with pytest.raises(DuneError, match="[Nn]ot implemented"):
        helper.pre_adapt()


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
