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
"""DiscreteBochnerFunction: constructor naming fallback and evaluation in time.

A DiscreteBochnerFunction (dune/gdt/discretefunction/bochner.hh) pairs a spatial DoF vector per
temporal node. Both the freshly-allocated factory (``make_discrete_bochner_function``) and the
list-of-vectors factory feed the constructor's ``name.empty() ? "DiscreteBochnerFunction" : name``
fallback; the existing call sites only ever pass a non-empty name, so the empty-string branch is
never taken. Here both sides are exercised on hypothesis-generated grids: an explicit non-empty name
is kept verbatim, while an empty name falls back to the class default. evaluate(time) is pinned to
its piecewise linear (P1-in-time) semantics, which also runs the temporal space's mapper through
the DynamicVector-returning ``global_indices`` overload of the mapper interface.
"""

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

# time_points define the internal 1d temporal Lagrange grid of the Bochner space; evenly spaced
# points over [0, end] keep every temporal interval well conditioned (no degenerate 1d cells).
TIME_POINTS = st.builds(
    lambda intervals, end: [end * i / intervals for i in range(intervals + 1)],
    intervals=st.integers(1, 4),
    end=st.floats(0.5, 4.0, allow_nan=False, allow_infinity=False),
)
NAMES = st.text(
    alphabet=st.characters(min_codepoint=48, max_codepoint=122), min_size=1, max_size=16
)


def _bochner_space(spec, time_points):
    from dune.gdt import BochnerSpace, ContinuousLagrangeSpace

    grid = spec.make_grid()
    spatial_space = ContinuousLagrangeSpace(grid, order=1)
    return BochnerSpace(spatial_space, list(time_points))


@given(spec=grid_specs(max_elements_per_dim=2), time_points=TIME_POINTS, name=NAMES)
def test_explicit_name_is_kept(spec, time_points, name):
    from dune.gdt import DiscreteBochnerFunction

    bochner_space = _bochner_space(spec, time_points)
    function = DiscreteBochnerFunction(bochner_space, name=name)
    # a non-empty name takes the (name.empty() ? default : name) false branch and is stored verbatim.
    assert function.name == name


@given(spec=grid_specs(max_elements_per_dim=2), time_points=TIME_POINTS)
def test_empty_name_falls_back_to_the_class_default(spec, time_points):
    from dune.gdt import DiscreteBochnerFunction

    bochner_space = _bochner_space(spec, time_points)
    function = DiscreteBochnerFunction(bochner_space, name="")
    # an empty name takes the true branch of the fallback and yields the class default name.
    assert function.name == "DiscreteBochnerFunction"


@given(
    spec=grid_specs(max_elements_per_dim=2),
    time_points=TIME_POINTS,
    data=st.data(),
)
def test_evaluate_interpolates_linearly_in_time(spec, time_points, data):
    """DiscreteBochnerFunction.evaluate(time) locates the temporal interval via the temporal
    (order 1 Lagrange) space's mapper -- the DynamicVector-returning global_indices overload of
    MapperInterface (dune/gdt/spaces/mapper/interfaces.hh), unreachable through any other
    binding -- and combines the two adjacent spatial vectors with the P1 hat function weights,
    i.e. linearly in time."""
    import numpy as np

    from dune.gdt import DiscreteBochnerFunction
    from dune.xt.la import Istl, IstlVector

    bochner_space = _bochner_space(spec, time_points)
    num_spatial_dofs = bochner_space.spatial_space.num_DoFs
    # one constant-valued spatial vector per time point, with drawn values
    values = data.draw(
        st.lists(
            st.floats(-1e3, 1e3, allow_nan=False, allow_infinity=False),
            min_size=len(time_points),
            max_size=len(time_points),
        ),
        label="values",
    )
    vectors = [IstlVector(num_spatial_dofs, value) for value in values]
    function = DiscreteBochnerFunction(bochner_space, vectors, Istl())

    # at the time points themselves the stored vectors are reproduced ...
    for time, value in zip(time_points, values, strict=True):
        assert np.allclose(
            np.array(function.evaluate(time).dofs.vector), value, atol=1e-10
        )
    # ... and inside a drawn interval the two adjacent vectors are combined linearly
    index = data.draw(st.integers(0, len(time_points) - 2), label="interval")
    ratio = data.draw(
        st.floats(0.1, 0.9, allow_nan=False, allow_infinity=False), label="ratio"
    )
    time = time_points[index] + ratio * (time_points[index + 1] - time_points[index])
    expected = (1 - ratio) * values[index] + ratio * values[index + 1]
    scale = max(1.0, abs(expected))
    assert np.allclose(
        np.array(function.evaluate(time).dofs.vector), expected, atol=1e-9 * scale
    )


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
