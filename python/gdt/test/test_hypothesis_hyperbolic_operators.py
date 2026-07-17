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
