# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2020)
# ~~~


def test_types():
    from dune.xt.grid import (
        AbsorbingBoundary,
        DirichletBoundary,
        ImpermeableBoundary,
        InflowBoundary,
        InflowOutflowBoundary,
        NeumannBoundary,
        NoBoundary,
        OutflowBoundary,
        ReflectingBoundary,
        RobinBoundary,
        UnknownBoundary,
    )

    NoBoundary()
    UnknownBoundary()
    DirichletBoundary()
    NeumannBoundary()
    RobinBoundary()
    ReflectingBoundary()
    AbsorbingBoundary()
    InflowBoundary()
    OutflowBoundary()
    InflowOutflowBoundary()
    ImpermeableBoundary()


def test_initless_boundary_infos():
    from grid_provider_cube import init_args as grid_init_args

    from dune.xt.grid import (
        AllDirichletBoundaryInfo,
        AllNeumannBoundaryInfo,
        AllReflectingBoundaryInfo,
        make_cube_grid,
    )

    for args in grid_init_args:
        grid = make_cube_grid(*args)
        AllDirichletBoundaryInfo(grid)
        AllNeumannBoundaryInfo(grid)
        AllReflectingBoundaryInfo(grid)


def test_normalbased_boundary_inf():
    from grid_provider_cube import init_args as grid_init_args

    from dune.xt.grid import NoBoundary, NormalBasedBoundaryInfo, make_cube_grid

    for args in grid_init_args:
        grid = make_cube_grid(*args)
        NormalBasedBoundaryInfo(
            grid_provider=grid,
            default_boundary_type=NoBoundary(),
            tolerance=1e-10,
            logging_prefix="",
        )
        NormalBasedBoundaryInfo(
            grid_provider=grid, default_boundary_type=NoBoundary(), tolerance=1e-10
        )
        NormalBasedBoundaryInfo(grid_provider=grid, default_boundary_type=NoBoundary())
