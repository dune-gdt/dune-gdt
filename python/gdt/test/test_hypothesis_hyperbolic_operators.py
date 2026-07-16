# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~
"""Property tests for AdvectionFvOperator / NumericalUpwindFlux / ExplicitEulerTimeStepper (#320 WP6).

Mirrors the C++ linear-transport FV test setup (dune/gdt/test/linear-transport/), but generates the
grid, transport velocity and CFL-respecting time step at runtime via hypothesis instead of compiling
one fixed combination per .cc file.
"""

import dataclasses

import pytest
from hypothesis import HealthCheck, given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    cfl_respecting_dt,
    constant_transport_velocities,
    fv_mass,
    gaussian_bump_expression,
    grid_specs,
    has_advection_fv,
    has_gdt_bindings,
    linear_transport_flux_expression,
)

pytestmark = pytest.mark.skipif(
    not has_advection_fv(),
    reason="this build does not bind AdvectionFvOperator (#320 WP6)",
)

# Minimum cells per axis: a too-coarse grid cannot resolve the Gaussian bump (sigma is set relative
# to the domain extent below) well enough for the mass-conservation comparison to be meaningful --
# an under-resolved bump's "mass" is numerical noise rather than a proper integral, which is exactly
# what a hypothesis-shrunk 1x3-element 2d grid found (mass ~1e-9, not conserved to abs=1e-12).
_MIN_ELEMENTS_PER_DIM = 24


@st.composite
def _linear_transport_cases(draw, dims=(1, 2)):
    spec = draw(
        grid_specs(
            dims=dims,
            elements=("cube",),
            max_elements_per_dim=_MIN_ELEMENTS_PER_DIM,
            unit_box=True,
            conforming_only=True,
        )
    )
    spec = dataclasses.replace(
        spec,
        num_elements=tuple(max(n, _MIN_ELEMENTS_PER_DIM) for n in spec.num_elements),
    )
    velocity = draw(constant_transport_velocities(spec.dim))
    return spec, velocity


@settings(
    max_examples=20,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(case=_linear_transport_cases())
def test_advection_fv_upwind_conserves_mass(case):
    """Sum(dofs * cell volume) is unchanged by explicit-Euler steps of the FV upwind operator.

    The initial data is a Gaussian bump centered in the domain, decayed to floating point noise well
    before the (non-periodic) boundary, so the boundary treatment contributes ~0 net flux and the
    interior numerical flux exactly cancels between neighbouring cells -- the discrete analogue of
    d/dt integral(u) = -boundary flux = 0.
    """
    from dune.gdt import (
        AdvectionFvOperator,
        ExplicitEulerTimeStepper,
        FiniteVolumeSpace,
        NumericalUpwindFlux,
        default_interpolation,
    )
    from dune.xt.functions import GridFunction

    spec, velocity = case
    grid = spec.make_grid()
    space = FiniteVolumeSpace(grid)

    half_extent = (
        min(hi - lo for lo, hi in zip(spec.lower_left, spec.upper_right, strict=True))
        / 2.0
    )
    # 8 sigma of margin to the boundary (exp(-32) of the peak) with _MIN_ELEMENTS_PER_DIM cells
    # comfortably resolving that same sigma -- see the comment on _MIN_ELEMENTS_PER_DIM above.
    sigma = half_extent / 8.0
    center = tuple(
        (lo + hi) / 2.0
        for lo, hi in zip(spec.lower_left, spec.upper_right, strict=True)
    )
    bump = gaussian_bump_expression(spec.dim, center, sigma)
    u_0 = default_interpolation(GridFunction(grid, bump), space)

    numerical_flux = NumericalUpwindFlux(
        grid, linear_transport_flux_expression(velocity)
    )
    op = AdvectionFvOperator(space, numerical_flux)
    # zero-order extrapolation, see advection-fv_for_all_grids.hh
    op.boundary_treatment(lambda u: u)

    dt = cfl_respecting_dt(spec, velocity, cfl=0.4)
    # op.apply(u) computes +div(f(u)); u_t + div(f(u)) = 0 requires r = -1 (irrelevant to mass
    # conservation itself, which holds by flux-cancellation regardless of sign, but required for
    # the scheme to actually be the stable upwind discretization rather than an unstable downwind one)
    time_stepper = ExplicitEulerTimeStepper(op, u_0, r=-1.0)

    mass_before = fv_mass(u_0, grid)
    for _ in range(5):
        time_stepper.step(dt)
    mass_after = fv_mass(time_stepper.current_solution(), grid)

    assert mass_after == pytest.approx(mass_before, rel=1e-6, abs=1e-12)


def _fv_linear_transport_setup(case):
    """Grid, FV space, interpolated Gaussian initial data and upwind FV operator for a drawn case."""
    from dune.gdt import (
        AdvectionFvOperator,
        FiniteVolumeSpace,
        NumericalUpwindFlux,
        default_interpolation,
    )
    from dune.xt.functions import GridFunction

    spec, velocity = case
    grid = spec.make_grid()
    space = FiniteVolumeSpace(grid)
    half_extent = (
        min(hi - lo for lo, hi in zip(spec.lower_left, spec.upper_right)) / 2.0
    )
    sigma = (
        half_extent / 8.0
    )  # see the comment in test_advection_fv_upwind_conserves_mass
    center = tuple((lo + hi) / 2.0 for lo, hi in zip(spec.lower_left, spec.upper_right))
    bump = gaussian_bump_expression(spec.dim, center, sigma)
    u_0 = default_interpolation(GridFunction(grid, bump), space)
    op = AdvectionFvOperator(
        space, NumericalUpwindFlux(grid, linear_transport_flux_expression(velocity))
    )
    op.boundary_treatment(lambda u: u)
    return grid, space, bump, u_0, op


@pytest.mark.skipif(
    not has_gdt_bindings("AdvectionDgOperator", "ExplicitEulerTimeStepper"),
    reason="this build does not bind AdvectionDgOperator (#320 WP6 follow-up)",
)
@settings(
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(
    case=_linear_transport_cases(),
    value=st.floats(min_value=0.5, max_value=2.0, allow_nan=False),
)
def test_advection_dg_preserves_constants(case, value):
    """Explicit Euler steps of the DG operator leave a constant state unchanged.

    For constant u the volume term -int_K f(u) grad(phi) and the surface terms int_dK F(u, u) n phi
    cancel exactly by the divergence theorem, PROVIDED every boundary intersection contributes --
    which is what boundary_treatment(u -> u) (zero-order extrapolation, so F(u_inside, u_inside)
    again) ensures. This exercises the whole assemble/local mass matrix inversion/apply pipeline.
    """
    import numpy as np

    from dune.gdt import (
        AdvectionDgOperator,
        DiscontinuousLagrangeSpace,
        ExplicitEulerTimeStepper,
        NumericalUpwindFlux,
        default_interpolation,
    )
    from dune.xt.functions import ExpressionFunction, GridFunction
    from dune.xt.grid import Dim

    spec, velocity = case
    grid = spec.make_grid()
    space = DiscontinuousLagrangeSpace(grid, order=1)

    constant = ExpressionFunction(
        dim_domain=Dim(spec.dim),
        variable="x",
        expression=f"{value!r}",
        order=0,
        name="constant_state",
    )
    u_0 = default_interpolation(GridFunction(grid, constant), space)

    op = AdvectionDgOperator(
        space, NumericalUpwindFlux(grid, linear_transport_flux_expression(velocity))
    )
    op.boundary_treatment(lambda u: u)
    op.assemble()  # unlike the FV operator, the DG operator precomputes local mass matrices

    # the DG CFL condition is stricter than the FV one by ~1/(2 order + 1)
    dt = cfl_respecting_dt(spec, velocity, cfl=0.4) / 3.0
    time_stepper = ExplicitEulerTimeStepper(op, u_0, r=-1.0)
    for _ in range(3):
        time_stepper.step(dt)

    dofs = np.asarray(time_stepper.current_solution().dofs.vector, dtype=float)
    assert np.max(np.abs(dofs - value)) == pytest.approx(0.0, abs=1e-10 * abs(value))


@pytest.mark.skipif(
    not has_gdt_bindings("AdaptiveRungeKuttaTimeStepper"),
    reason="this build does not bind the adaptive time steppers (#320 WP6 follow-up)",
)
@settings(
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(case=_linear_transport_cases())
def test_adaptive_rungekutta_conserves_mass(case):
    """Dormand-Prince (RK45) steps of the FV upwind operator conserve sum(dofs * cell volume).

    Every stage is an application of the mass-conserving FV operator and every (possibly rejected
    and retried) update is a linear combination of stages, so the adaptive error control must not
    break conservation.
    """
    from dune.gdt import DormandPrinceTimeStepper

    spec, velocity = case
    grid, _, _, u_0, op = _fv_linear_transport_setup(case)

    dt = cfl_respecting_dt(spec, velocity, cfl=0.4)
    time_stepper = DormandPrinceTimeStepper(op, u_0, r=-1.0, tol=1e-4)

    mass_before = fv_mass(u_0, grid)
    suggested_dt = dt
    for _ in range(3):
        suggested_dt = time_stepper.step(min(suggested_dt, dt), dt)
    mass_after = fv_mass(time_stepper.current_solution(), grid)

    assert mass_after == pytest.approx(mass_before, rel=1e-6, abs=1e-12)


@pytest.mark.skipif(
    not has_gdt_bindings("StrangSplittingTimeStepper", "FractionalStepTimeStepper"),
    reason="this build does not bind the splitting time steppers (#320 WP6 follow-up)",
)
@settings(
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(
    case=_linear_transport_cases(dims=(2,)),
    splitting=st.sampled_from(["fractional_step", "strang"]),
)
def test_splitting_time_steppers_conserve_mass(case, splitting):
    """Dimensional splitting v = (v_0, 0) + (0, v_1) of 2d transport conserves the discrete mass.

    Each substepper advances the SAME DiscreteFunction with a mass-conserving FV upwind operator for
    one of the two directional fluxes, so the composed (Godunov or Strang) scheme has to conserve
    sum(dofs * cell volume) as well.
    """
    from dune.gdt import (
        AdvectionFvOperator,
        ExplicitEulerTimeStepper,
        FiniteVolumeSpace,
        FractionalStepTimeStepper,
        NumericalUpwindFlux,
        StrangSplittingTimeStepper,
        default_interpolation,
    )
    from dune.xt.functions import GridFunction

    spec, velocity = case
    grid = spec.make_grid()
    space = FiniteVolumeSpace(grid)
    half_extent = (
        min(hi - lo for lo, hi in zip(spec.lower_left, spec.upper_right)) / 2.0
    )
    sigma = half_extent / 8.0
    center = tuple((lo + hi) / 2.0 for lo, hi in zip(spec.lower_left, spec.upper_right))
    bump = gaussian_bump_expression(spec.dim, center, sigma)
    u_0 = default_interpolation(GridFunction(grid, bump), space)

    steppers = []
    for directional_velocity in ((velocity[0], 0.0), (0.0, velocity[1])):
        op = AdvectionFvOperator(
            space,
            NumericalUpwindFlux(
                grid, linear_transport_flux_expression(directional_velocity)
            ),
        )
        op.boundary_treatment(lambda u: u)
        # both substeppers evolve the same DiscreteFunction, see FractionalTimeStepper's constructor
        steppers.append(ExplicitEulerTimeStepper(op, u_0, r=-1.0))

    make_stepper = (
        FractionalStepTimeStepper
        if splitting == "fractional_step"
        else StrangSplittingTimeStepper
    )
    time_stepper = make_stepper(steppers[0], steppers[1])

    dt = cfl_respecting_dt(spec, velocity, cfl=0.4)
    mass_before = fv_mass(u_0, grid)
    for _ in range(3):
        time_stepper.step(dt)
    mass_after = fv_mass(time_stepper.current_solution(), grid)

    assert mass_after == pytest.approx(mass_before, rel=1e-6, abs=1e-12)


@pytest.mark.skipif(
    not has_gdt_bindings("estimate_dt_for_hyperbolic_system"),
    reason="this build does not bind estimate_dt_for_hyperbolic_system (#320 WP6 follow-up)",
)
@settings(
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(case=_linear_transport_cases(dims=(1,)))
def test_estimate_dt_matches_hand_computed_bound(case):
    """[CCL1995] dt estimate for 1d linear transport on an equidistant grid is h / (2 |v|).

    For f(u) = v u the maximal flux derivative is |v| independently of the state's data range, and
    every cell of an equidistant 1d grid has perimeter/volume = 2/h, so the estimate
    1 / (perimeter/volume * max|f'|) can be verified exactly.
    """
    from dune.gdt import estimate_dt_for_hyperbolic_system

    spec, velocity = case
    grid, _, _, u_0, _ = _fv_linear_transport_setup(case)

    dt = estimate_dt_for_hyperbolic_system(
        grid, u_0, linear_transport_flux_expression(velocity)
    )

    h = (spec.upper_right[0] - spec.lower_left[0]) / spec.num_elements[0]
    assert dt == pytest.approx(h / (2.0 * abs(velocity[0])), rel=1e-6)


@pytest.mark.skipif(
    not has_gdt_bindings("LinearReconstructionOperator", "DiscontinuousLagrangeSpace"),
    reason="this build does not bind LinearReconstructionOperator (#320 WP6 follow-up)",
)
@settings(
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow, HealthCheck.filter_too_much],
)
@given(
    case=_linear_transport_cases(dims=(1,)),
    slope=st.sampled_from(["minmod", "mc", "superbee", "central", "no_reconstruction"]),
)
def test_linear_reconstruction_preserves_cell_averages(case, slope):
    """The reconstructed order-1 DG function has the same cell averages, hence the same mass.

    Slope limiting tilts the FV cell averages without moving them: on every 1d cell the
    reconstruction is u_i + slope * (x - x_i), whose average is u_i again. With 2 (Lagrange) dofs
    per 1d cell of an equidistant grid, sum over all dg dofs * h/2 is exactly the integral of the
    reconstruction, which therefore has to agree with fv_mass of the input for every slope choice.
    """
    import numpy as np

    from dune.gdt import LinearReconstructionOperator

    spec, velocity = case
    grid, space, bump, u_0, _ = _fv_linear_transport_setup(case)

    # the Gaussian bump also serves as the (numerically zero at the boundary) boundary values that
    # fill the reconstruction stencils of the two boundary cells
    op = LinearReconstructionOperator(
        space, linear_transport_flux_expression(velocity), bump, slope=slope
    )
    reconstructed = np.asarray(op.apply(u_0.dofs.vector), dtype=float)

    h = (spec.upper_right[0] - spec.lower_left[0]) / spec.num_elements[0]
    mass_fv = fv_mass(u_0, grid)
    mass_reconstructed = float(np.sum(reconstructed) * h / 2.0)

    assert mass_reconstructed == pytest.approx(mass_fv, rel=1e-10, abs=1e-14)
