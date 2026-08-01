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
"""#393: k3d (and matplotlib, which dune.xt.common.vtk.plot itself unconditionally needs -- see the
pyproject.toml comments) are now in the ctest `test` dependency group, so the two Python-facade
visualize entry points gated on HAVE_K3D -- dune.xt.grid.visualize_grid and
dune.xt.functions.visualize_function -- are reachable under test for the first time. Both ultimately
hand a VTK file off to dune.xt.common.vtk.plot.plot (covered directly, with synthetic files, in
test_vtk_plot.py); this drives them through their own grid-walking / file-writing halves with real
bound grid and function objects instead.
"""

import pytest

from dune.xt.common.config import config

grid = pytest.importorskip("dune.xt.grid")
functions = pytest.importorskip("dune.xt.functions")

# Only the k3d branches actually need it: dune.xt.grid.visualize_grid's 1d path (matplotlib) never
# reaches its `elif config.HAVE_K3D` sibling, and dune.xt.functions.visualize_function itself is
# undefined without HAVE_K3D -- gate per-test rather than module-wide so the 1d path still gets
# exercised on builds without k3d.
requires_k3d = pytest.mark.skipif(
    not config.HAVE_K3D, reason="k3d not available in this build"
)


def _skip_unless_cube_grid_available():
    if not hasattr(grid, "make_cube_grid"):
        pytest.skip("no make_cube_grid binding available in this build")


@requires_k3d
@pytest.mark.parametrize("dim", (2, 3))
def test_visualize_grid_returns_k3d_plot(dim, cube_grid):
    _skip_unless_cube_grid_available()
    from dune.xt.common.vtk.plot import VTKPlot

    provider = cube_grid(dim)
    result = grid.visualize_grid(provider)
    assert isinstance(result, VTKPlot)


def test_visualize_grid_1d_uses_matplotlib_not_k3d(cube_grid):
    _skip_unless_cube_grid_available()
    from matplotlib.axes import Axes

    # the 1d branch in dune.xt.grid.visualize_grid returns before ever checking HAVE_K3D
    provider = cube_grid(1, num_elements=4)
    ax = grid.visualize_grid(provider)
    assert isinstance(ax, Axes)


def _skip_unless_expression_function_available():
    _skip_unless_cube_grid_available()
    if not hasattr(functions, "ExpressionFunction"):
        pytest.skip("no ExpressionFunction binding available in this build")


@requires_k3d
@pytest.mark.parametrize("dim", (2, 3))
def test_visualize_function_returns_k3d_plot(dim, cube_grid):
    _skip_unless_expression_function_available()
    from dune.xt.common.vtk.plot import VTKPlot
    from dune.xt.grid import Dim

    provider = cube_grid(dim)
    func = functions.ExpressionFunction(
        dim_domain=Dim(dim),
        variable="x",
        order=2,
        expression="x[0]*x[0]",
        name="quadratic",
    )
    result = functions.visualize_function(func, provider)
    assert isinstance(result, VTKPlot)


@requires_k3d
def test_visualize_function_accepts_a_list_of_functions(cube_grid):
    """A list writes a .pvd time-series collection instead of a single .vtu (see the `len(functions)
    == 1` branch in dune.xt.functions.visualize_function) -- exercise that path too."""
    _skip_unless_expression_function_available()
    from dune.xt.common.vtk.plot import VTKPlot
    from dune.xt.grid import Dim

    provider = cube_grid(2)
    functions_list = [
        functions.ExpressionFunction(
            dim_domain=Dim(2), variable="x", order=1, expression=expr, name="f"
        )
        for expr in ("x[0]", "x[1]")
    ]
    result = functions.visualize_function(functions_list, provider)
    assert isinstance(result, VTKPlot)
    # one timestep per function, not just a single collapsed frame
    assert len(result.vtk_data) == len(functions_list)
