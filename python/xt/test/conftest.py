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

import builtins
import os

import pytest
from hypothesis import HealthCheck, settings
from hypothesis.errors import InvalidArgument

# Every example crosses into compiled dune code (grid construction, assembly); wall time per
# example is dominated by C++, not by data generation, and varies wildly between machines and
# ccache states. Hypothesis' 200ms deadline is meant to catch pathological *generation*, so it
# is disabled here; the example counts keep total runtime bounded instead.
settings.register_profile(
    "dune-ci",
    max_examples=25,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
)
settings.register_profile(
    "dune-dev",
    max_examples=10,
    deadline=None,
    suppress_health_check=[HealthCheck.too_slow],
)
try:
    settings.load_profile(os.environ.get("HYPOTHESIS_PROFILE", "dune-ci"))
except InvalidArgument:
    # HYPOTHESIS_PROFILE names a profile this conftest does not know (e.g. one registered by a
    # developer's own tooling that is not on this run's plugin path); fall back instead of
    # failing collection of the whole suite
    settings.load_profile("dune-ci")


@pytest.fixture(autouse=True)
def _k3d_display_builtin(monkeypatch):
    """k3d's Plot.display() (k3d/plot/plot_display.py) calls a bare `display(...)` name. A real
    Jupyter frontend gets this for free -- IPython.core.interactiveshell.InteractiveShell installs
    it into `builtins` during init -- but under ctest there is no kernel to do that, so plot()
    would otherwise raise `NameError: name 'display' is not defined` (see test_vtk_plot.py,
    test_visualize.py, #393). Providing it here rather than per test module is a no-op for every
    test that never touches k3d, and avoids standing up a whole InteractiveShell, which would
    mutate process-global state (sys.displayhook/excepthook, builtin traps) for the rest of the
    session.
    """
    from IPython.display import display

    monkeypatch.setattr(builtins, "display", display, raising=False)


@pytest.fixture
def cube_grid():
    """Factory for a `dim`-dimensional unit-cube grid, shared by test_visualize.py and
    test_hypothesis_functions.py's own local helper of the same shape."""

    def _make(dim, num_elements=2):
        from dune.xt.grid import Cube, Dim, make_cube_grid

        return make_cube_grid(
            Dim(dim),
            Cube(),
            lower_left=[0.0] * dim,
            upper_right=[1.0] * dim,
            num_elements=[num_elements] * dim,
        )

    return _make
