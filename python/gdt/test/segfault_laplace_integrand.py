# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2020)
#   René Fritze     (2022)
#   Tim Keil        (2022)
#
# flake8: noqa
# silencing all warnings here, in case import /instruction order was the trigger for the segfault
# ~~~

from dune.xt.grid import Dim, Cube, make_cube_grid

def test_segfault():
    grid = make_cube_grid(Dim(1), [0], [1], [2])
    d = grid.dimension

    from dune.xt.functions import ConstantFunction, GridFunction
    from dune.xt.la import Istl
    from dune.gdt import ContinuousLagrangeSpace, MatrixOperator, LocalElementIntegralBilinearForm, LocalLaplaceIntegrand, BilinearForm

    space = ContinuousLagrangeSpace(grid, 1)
    op = MatrixOperator(grid, space, space, Istl())

    func = ConstantFunction(Dim(1), Dim(1), [1])
    aform = BilinearForm(grid)
    aform += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GridFunction(grid, func, (Dim(d), Dim(d)))))
    op.append(aform)
    del func
    op.assemble()
