# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)

# flake8: noqa

import numpy as np

from dune.xt.grid import Dim, Simplex, make_cube_grid
from dune.xt.functions import ExpressionFunction, GridFunction as GF


def _products(grid):
    from dune.gdt import (
        BilinearForm,
        LocalElementIntegralBilinearForm,
        LocalElementProductIntegrand,
        LocalLaplaceIntegrand,
    )

    d = grid.dimension

    L2_product = BilinearForm(grid)
    L2_product += LocalElementIntegralBilinearForm(
        LocalElementProductIntegrand(GF(grid, 1))
    )

    H1_semi_product = BilinearForm(grid)
    H1_semi_product += LocalElementIntegralBilinearForm(
        LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d))))
    )

    H1_product = BilinearForm(grid)
    H1_product += LocalElementIntegralBilinearForm(
        LocalElementProductIntegrand(GF(grid, 1))
    )
    H1_product += LocalElementIntegralBilinearForm(
        LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d))))
    )

    return L2_product, H1_semi_product, H1_product


def test_bilinear_form_apply2_and_norm():
    grid = make_cube_grid(
        Dim(2), Simplex(), lower_left=[0, 0], upper_right=[1, 1], num_elements=[4, 4]
    )
    grid.global_refine(1)
    d = grid.dimension

    from dune.gdt import ContinuousLagrangeSpace, default_interpolation

    space = ContinuousLagrangeSpace(grid, order=1)
    u_h = default_interpolation(
        GF(
            grid,
            ExpressionFunction(
                dim_domain=Dim(d),
                variable="x",
                order=2,
                expression="x[0]*x[1]",
                name="u",
            ),
        ),
        space,
    )
    # wrap the DiscreteFunction as a GridFunction (the argument type expected by apply2/norm)
    u = GF(grid, u_h)

    L2_product, H1_semi_product, H1_product = _products(grid)

    # apply2(u, u) computes a(u, u); each product is symmetric positive (semi-)definite
    l2 = L2_product.apply2(u, u)
    h1_semi = H1_semi_product.apply2(u, u)
    h1 = H1_product.apply2(u, u)

    assert l2 > 0
    assert h1_semi > 0
    assert h1 > 0

    # H1 product is the sum of the L2 product and the H1 semi product
    assert np.isclose(l2 + h1_semi, h1)

    # norm(u) == sqrt(apply2(u, u))
    assert np.isclose(L2_product.norm(u), np.sqrt(l2))
    assert np.isclose(H1_semi_product.norm(u), np.sqrt(h1_semi))
    assert np.isclose(H1_product.norm(u), np.sqrt(h1))
