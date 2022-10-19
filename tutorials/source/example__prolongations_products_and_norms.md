```
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
```

---
jupytext:
  text_representation:
   format_name: myst
jupyter:
  jupytext:
    cell_metadata_filter: -all
    formats: ipynb,myst
    main_language: python
    text_representation:
      format_name: myst
      extension: .md
      format_version: '1.3'
      jupytext_version: 1.11.2
kernelspec:
  display_name: Python 3
  name: python3
---

```{try_on_binder}
```

```{code-cell}
:tags: [remove-cell]
:load: myst_code_init.py
```

# Example: prolongations and computing products and norms

## This is work in progress (WIP), still missing:

* documentation and explanation
* assembled matrix as product

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer
%matplotlib notebook

import numpy as np
np.warnings.filterwarnings('ignore') # silence numpys warnings
```

## 1: prolongation onto a finer grid

Let's set up a 2d grid first, as seen in other tutorials and examples.

```{code-cell}
from dune.xt.grid import Dim, Cube, Simplex, make_cube_grid
from dune.xt.functions import ExpressionFunction, GridFunction as GF

d = 2
omega = ([0, 0], [1, 1])
grid = make_cube_grid(Dim(d), Simplex(), lower_left=omega[0], upper_right=omega[1], num_elements=[2, 2])
grid.global_refine(1)

print(f'grid has {grid.size(0)} elements, {grid.size(d - 1)} edges and {grid.size(d)} vertices')
```

```{code-cell}
from dune.gdt import default_interpolation, prolong, DiscontinuousLagrangeSpace

V_H = DiscontinuousLagrangeSpace(grid, order=1)

f = ExpressionFunction(dim_domain=Dim(d), variable='x', order=10, expression='exp(x[0]*x[1])', name='f')
f_H = default_interpolation(GF(grid, f), V_H)

print(f'f_H has {len(f_H.dofs.vector)} DoFs')
```

```{code-cell}
reference_grid = make_cube_grid(Dim(d), Simplex(), lower_left=omega[0], upper_right=omega[1], num_elements=[16, 16])
reference_grid.global_refine(1)

print(f'grid has {reference_grid.size(0)} elements')
```

```{code-cell}
from dune.gdt import DiscreteFunction

V_h = DiscontinuousLagrangeSpace(reference_grid, order=1)

f_h = prolong(f_H, V_h)
print(f'f_h has {len(f_h.dofs.vector)} DoFs')
```

## 2: computing products and norms

```{code-cell}
u = GF(grid,
       ExpressionFunction(dim_domain=Dim(d), variable='x', order=10, expression='exp(x[0]*x[1])',
                          gradient_expressions=['x[1]*exp(x[0]*x[1])', 'x[0]*exp(x[0]*x[1])'], name='h'))
```

$$
(u, v)_{H^1} = (u, v)_{L^2} + (u, v)_{H^1_0}
$$

$$
||u||_{H^1} = \sqrt{||u||_{L^2}^2 + |u|_{H^1_0}^2}
$$


Either comupte all in one grid walk ...

```{code-cell}
from dune.xt.grid import Walker

from dune.gdt import (
    BilinearForm,
    LocalElementProductIntegrand,
    LocalElementIntegralBilinearForm,
    LocalLaplaceIntegrand,
)

L2_product = BilinearForm(grid, u, u)
L2_product += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))

H1_semi_product = BilinearForm(grid, u, u)
H1_semi_product += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d)))))

H1_product = BilinearForm(grid, u, u)
H1_product += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))
H1_product += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d)))))

walker = Walker(grid)
walker.append(L2_product)
walker.append(H1_semi_product)
walker.append(H1_product)
walker.walk()

print(f'|| u ||_L2 = {np.sqrt(L2_product.result)}')
print(f' | u |_H1  = {np.sqrt(H1_semi_product.result)}')
print(f'|| u ||_H1 = {np.sqrt(H1_product.result)}')
```

```{code-cell}
np.allclose(L2_product.result + H1_semi_product.result, H1_product.result)
```

... or let each product do the grid walking in `apply2` (possibly less runtime efficient, but maybe more intuitive)

```{code-cell}
L2_product = BilinearForm(grid, u, u)
L2_product += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))
print(f'|| u ||_L2 = {np.sqrt(L2_product.apply2())}')

H1_semi_product = BilinearForm(grid, u, u)
H1_semi_product += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d)))))
print(f' | u |_H1  = {np.sqrt(H1_semi_product.apply2())}')

H1_product = BilinearForm(grid, u, u)
H1_product += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))
H1_product += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GF(grid, 1, dim_range=(Dim(d), Dim(d)))))
print(f'|| u ||_H1 = {np.sqrt(H1_product.apply2())}')
```

## 3: computing errors w.r.t. a known reference solution

```{code-cell}
# suppose we have a DiscreteFunction on a coarse grid, usually the solution of a discretization
# (we use the interpolation here for simplicity)

u_H = default_interpolation(u, V_H)

# prolong it onto the reference grid
u_h = prolong(u_H, V_h)

# define error function using the analytically known solution u
error = GF(grid, u - u_h)

# compute norms on reference grid
L2_product = BilinearForm(reference_grid, error, error)
L2_product += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1)))
print(f'|| error ||_L2 = {np.sqrt(L2_product.apply2())}')
```

Download the code:
{download}`example__prolongations_products_and_norms.md`
{nb-download}`example__prolongations_products_and_norms.ipynb`
