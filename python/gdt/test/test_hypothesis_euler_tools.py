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
"""Property tests for the EulerTools bindings (#320 WP6 follow-up).

EulerTools is purely algebraic (no grid involved), so its conversions and the eigendecomposition of
the directional flux Jacobian can be verified exactly against their defining identities.
"""

import math

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import has_gdt_bindings

pytestmark = pytest.mark.skipif(
    not has_gdt_bindings("EulerTools"),
    reason="this build does not bind EulerTools (#320 WP6 follow-up)",
)

# The flux Jacobian and its eigendecomposition are implemented for d in {1, 2} only (3d raises
# NotImplemented, matching dune/gdt/tools/euler.hh).
_DIMS = (1, 2)

_positive = st.floats(min_value=0.1, max_value=10.0, allow_nan=False)
_velocity_component = st.floats(min_value=-5.0, max_value=5.0, allow_nan=False)
_gamma = st.floats(min_value=1.1, max_value=2.0, allow_nan=False)


@st.composite
def _euler_states(draw, dims=_DIMS):
    dim = draw(st.sampled_from(dims))
    rho = draw(_positive)
    v = tuple(draw(_velocity_component) for _ in range(dim))
    p = draw(_positive)
    gamma = draw(_gamma)
    return dim, rho, v, p, gamma


@st.composite
def _unit_normals(draw, dim):
    direction = draw(
        st.tuples(*[_velocity_component for _ in range(dim)]).filter(
            lambda n: sum(c * c for c in n) > 1e-2
        )
    )
    norm = math.sqrt(sum(c * c for c in direction))
    return tuple(c / norm for c in direction)


@settings(max_examples=50, deadline=None)
@given(state=_euler_states())
def test_primitive_conservative_roundtrip(state):
    """to_primitive(to_conservative(rho, v, p)) == (rho, v, p), and the accessors agree."""
    from dune.gdt import EulerTools

    dim, rho, v, p, gamma = state
    tools = EulerTools(dim, gamma=gamma)

    w = tools.to_conservative(rho, list(v), p)
    assert len(w) == dim + 2

    primitive = tools.to_primitive(w)
    assert primitive[tools.density_index] == pytest.approx(rho, rel=1e-12)
    for i, vi in zip(tools.velocity_indices, v):
        assert primitive[i] == pytest.approx(vi, rel=1e-12, abs=1e-12)
    assert primitive[tools.pressure_index] == pytest.approx(p, rel=1e-12)

    assert tools.density(w) == pytest.approx(rho, rel=1e-12)
    assert tuple(tools.velocity(w)) == pytest.approx(v, rel=1e-12, abs=1e-12)
    assert tools.pressure(w) == pytest.approx(p, rel=1e-12)
    # E = p / (gamma - 1) + rho |v|^2 / 2
    energy = p / (gamma - 1.0) + 0.5 * rho * sum(c * c for c in v)
    assert tools.energy(w) == pytest.approx(energy, rel=1e-12)
    assert tools.speed_of_sound(w) == pytest.approx(
        math.sqrt(gamma * p / rho), rel=1e-12
    )
    assert tools.mach_number(w) == pytest.approx(
        math.sqrt(sum(c * c for c in v)) / math.sqrt(gamma * p / rho),
        rel=1e-12,
        abs=1e-12,
    )


@settings(max_examples=50, deadline=None)
@given(data=st.data(), state=_euler_states())
def test_flux_jacobian_eigendecomposition(data, state):
    """T diag(lambda) T^{-1} == sum_s n_s (f_s)'(w), the identity the eigendecomposition claims."""
    from dune.gdt import EulerTools

    dim, rho, v, p, gamma = state
    n = data.draw(_unit_normals(dim))
    tools = EulerTools(dim, gamma=gamma)
    w = tools.to_conservative(rho, list(v), p)

    jacobians = [np.asarray(jac, dtype=float) for jac in tools.flux_jacobian(w)]
    jacobian_times_n = sum(n_s * jac for n_s, jac in zip(n, jacobians))

    eigenvalues = np.asarray(tools.eigenvalues_flux_jacobian(w, list(n)), dtype=float)
    eigenvectors = np.asarray(tools.eigenvectors_flux_jacobian(w, list(n)), dtype=float)
    eigenvectors_inv = np.asarray(
        tools.eigenvectors_inv_flux_jacobian(w, list(n)), dtype=float
    )

    assert eigenvectors @ eigenvectors_inv == pytest.approx(np.eye(dim + 2), abs=1e-8)
    assert eigenvectors @ np.diag(eigenvalues) @ eigenvectors_inv == pytest.approx(
        jacobian_times_n, rel=1e-8, abs=1e-8
    )


@settings(max_examples=50, deadline=None)
@given(state=_euler_states())
def test_flux_of_conservative_state(state):
    """The bound flux matches its textbook definition f_s(w) = (rho v_s, rho v v_s + p e_s, (E+p) v_s)."""
    from dune.gdt import EulerTools

    dim, rho, v, p, gamma = state
    tools = EulerTools(dim, gamma=gamma)
    w = tools.to_conservative(rho, list(v), p)
    energy = tools.energy(w)

    flux = np.asarray(tools.flux(w), dtype=float)
    assert flux.shape == (dim, dim + 2)
    for s in range(dim):
        expected = np.empty(dim + 2)
        expected[0] = rho * v[s]
        for i in range(dim):
            expected[1 + i] = rho * v[i] * v[s] + (p if i == s else 0.0)
        expected[dim + 1] = (energy + p) * v[s]
        assert flux[s] == pytest.approx(expected, rel=1e-12, abs=1e-12)
