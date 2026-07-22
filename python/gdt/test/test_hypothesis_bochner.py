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
"""DiscreteBochnerFunction naming: the default-name fallback of the constructors.

A DiscreteBochnerFunction (dune/gdt/discretefunction/bochner.hh) pairs a spatial DoF vector per
temporal node. Both the freshly-allocated factory (``make_discrete_bochner_function``) and the
list-of-vectors factory feed the constructor's ``name.empty() ? "DiscreteBochnerFunction" : name``
fallback; the existing call sites only ever pass a non-empty name, so the empty-string branch is
never taken. Here both sides are exercised on hypothesis-generated grids: an explicit non-empty name
is kept verbatim, while an empty name falls back to the class default.
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


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
