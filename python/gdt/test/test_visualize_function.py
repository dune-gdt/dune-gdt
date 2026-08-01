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
"""#393: k3d (and matplotlib, which dune.xt.common.vtk.plot itself unconditionally needs -- see the
pyproject.toml comments) are now in the ctest `test` dependency group, so dune.gdt.visualize_function
-- gated on HAVE_K3D and, until now, entirely untested -- is reachable. Covers both of its branches:
the 1d matplotlib path (a plain XT function interpolated onto a P1 space) and the 2d k3d path (a
DiscreteFunction, whose bound `.visualize(filename=...)` takes the "discrete function" try-branch
directly, see dune/gdt/__init__.py).
"""

import pytest

from dune.xt.common.config import config

def test_visualize_function_1d_uses_matplotlib():
    ...

`@pytest.mark.skipif`(not config.HAVE_K3D, reason="k3d not available in this build")
def test_visualize_function_2d_discrete_function_returns_k3d_plot():
    ...

gdt = pytest.importorskip("dune.gdt", exc_type=ImportError)
grid = pytest.importorskip("dune.xt.grid")
functions = pytest.importorskip("dune.xt.functions")


def _skip_unless_available():
    if not hasattr(grid, "make_cube_grid") or not hasattr(
        functions, "ExpressionFunction"
    ):
        pytest.skip(
            "no make_cube_grid/ExpressionFunction binding available in this build"
        )
    if not hasattr(gdt, "visualize_function"):
        pytest.skip("no visualize_function binding available in this build")


def test_visualize_function_1d_uses_matplotlib():
    _skip_unless_available()
    from matplotlib.axes import Axes
    from dune.xt.grid import Cube, Dim, make_cube_grid

    grid_1d = make_cube_grid(
        Dim(1), Cube(), lower_left=[0.0], upper_right=[1.0], num_elements=[4]
    )
    func = functions.ExpressionFunction(
        dim_domain=Dim(1),
        variable="x",
        order=2,
        expression="x[0]*x[0]",
        name="quadratic",
    )
    ax = gdt.visualize_function(func, grid=grid_1d)
    assert isinstance(ax, Axes)


def test_visualize_function_2d_discrete_function_returns_k3d_plot():
    _skip_unless_available()
    from dune.xt.common.vtk.plot import VTKPlot
    from dune.xt.functions import GridFunction as GF
    from dune.xt.grid import Cube, Dim, make_cube_grid

    grid_2d = make_cube_grid(
        Dim(2),
        Cube(),
        lower_left=[0.0, 0.0],
        upper_right=[1.0, 1.0],
        num_elements=[4, 4],
    )
    space = gdt.ContinuousLagrangeSpace(grid_2d, order=1)
    func = functions.ExpressionFunction(
        dim_domain=Dim(2),
        variable="x",
        order=2,
        expression="x[0]*x[1]",
        name="bilinear",
    )
    u_h = gdt.default_interpolation(GF(grid_2d, func), space)

    result = gdt.visualize_function(u_h)
    assert isinstance(result, VTKPlot)
