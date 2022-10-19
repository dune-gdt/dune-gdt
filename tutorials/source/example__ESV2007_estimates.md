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

# Example: a posteriori error estimates based on flux reconstruction

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer
%matplotlib notebook

import numpy as np
np.warnings.filterwarnings('ignore') # silence numpys warnings
```

```{code-cell}
from dune.xt.grid import Dim, Simplex, make_cube_grid, AllDirichletBoundaryInfo, visualize_grid

d = 2
grid = make_cube_grid(Dim(d), Simplex(), [-1, -1], [1, 1], [4, 4])
grid.global_refine(2)

print(f'grid has {grid.size(0)} elements, {grid.size(d - 1)} edges and {grid.size(d)} vertices')

boundary_info = AllDirichletBoundaryInfo(grid)
```

```{code-cell}
from dune.xt.functions import ExpressionFunction, GridFunction as GF

diffusion = GF(grid, 1., dim_range=(Dim(d), Dim(d)), name='diffusion')
source = GF(grid, ExpressionFunction(
    dim_domain=Dim(d), variable='x',
    expression='0.5*pi*pi*cos(0.5*pi*x[0])*cos(0.5*pi*x[1])', order=4, name='source'))
```

```{code-cell}
from dune.xt.grid import (
    ApplyOnCustomBoundaryIntersections,
    ApplyOnInnerIntersectionsOnce,
    DirichletBoundary,
    Walker,
)

from dune.gdt import (
    DiscontinuousLagrangeSpace,
    DiscreteFunction,
    LocalElementIntegralBilinearForm,
    LocalElementIntegralFunctional,
    LocalElementProductIntegrand,
    LocalCouplingIntersectionIntegralBilinearForm,
    LocalIPDGBoundaryPenaltyIntegrand,
    LocalIPDGInnerPenaltyIntegrand,
    LocalIntersectionIntegralBilinearForm,
    LocalLaplaceIntegrand,
    LocalLaplaceIPDGDirichletCouplingIntegrand,
    LocalLaplaceIPDGInnerCouplingIntegrand,
    MatrixOperator,
    VectorFunctional,
    estimate_combined_inverse_trace_inequality_constant,
    estimate_element_to_intersection_equivalence_constant,
    make_element_and_intersection_sparsity_pattern,
)

V_h = DiscontinuousLagrangeSpace(grid, order=1)
```

```{code-cell}
weight = diffusion
penalty_parameter = 16
symmetry_factor = 1

l_h = VectorFunctional(grid, V_h)
l_h += LocalElementIntegralFunctional(LocalElementProductIntegrand(GF(grid, 1)).with_ansatz(source))

a_h = MatrixOperator(grid, V_h, V_h, make_element_and_intersection_sparsity_pattern(V_h))
a_h += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(diffusion))
a_h += (LocalCouplingIntersectionIntegralBilinearForm(
            LocalLaplaceIPDGInnerCouplingIntegrand(symmetry_factor, diffusion, weight)
            + LocalIPDGInnerPenaltyIntegrand(penalty_parameter, weight)),
        {}, ApplyOnInnerIntersectionsOnce(grid))
a_h += (LocalIntersectionIntegralBilinearForm(
            LocalIPDGBoundaryPenaltyIntegrand(penalty_parameter, weight)
            + LocalLaplaceIPDGDirichletCouplingIntegrand(symmetry_factor, diffusion)),
       {}, ApplyOnCustomBoundaryIntersections(grid, boundary_info, DirichletBoundary()))

walker = Walker(grid)
walker.append(a_h)
walker.append(l_h)
walker.walk()
```

```{code-cell}
from dune.gdt import visualize_function

u_h = DiscreteFunction(V_h, 'u_h')

a_h.apply_inverse(l_h.vector, u_h.dofs.vector)

_ = visualize_function(u_h)
```

```{code-cell}
from dune.gdt import oswald_interpolation

s_h = oswald_interpolation(u_h, V_h)

_ = visualize_function(u_h - s_h, grid)
```

```{code-cell}
from dune.gdt import LaplaceIpdgFluxReconstructionOperator, RaviartThomasSpace

RT_0 = RaviartThomasSpace(grid, order=0)

flux_reconstruction = LaplaceIpdgFluxReconstructionOperator(
    grid, V_h, RT_0, symmetry_factor, penalty_parameter, penalty_parameter, diffusion, weight)
flux_reconstruction.assemble()

t_h = DiscreteFunction(RT_0, 't_h')
flux_reconstruction.apply(u_h.dofs.vector, t_h.dofs.vector)
```

```{code-cell}
from dune.gdt import FiniteVolumeSpace, Operator, LocalElementBilinearFormIndicatorOperator

fv_space = FiniteVolumeSpace(grid)

eta_nc_op = Operator(grid, V_h, fv_space)
eta_nc_op += LocalElementBilinearFormIndicatorOperator(
    LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(weight)))
eta_nc_2 = eta_nc_op.apply(u_h - s_h)
_ = visualize_function(eta_nc_2)
print(np.sqrt(eta_nc_2.dofs.vector.l1_norm()))
```

```{code-cell}
from dune.xt.functions import divergence, ElementwiseMinimumFunction, ElementwiseDiameterFunction, inverse

h = ElementwiseDiameterFunction(grid)
min_EV = ElementwiseMinimumFunction(diffusion, search_quadrature_order=0) # we know diffusion is constant
C_P = GF(grid, 1/np.pi**2) # known for simplices

eta_r_op = Operator(grid, V_h, fv_space)
eta_r_op += LocalElementBilinearFormIndicatorOperator(
    LocalElementIntegralBilinearForm(LocalElementProductIntegrand((C_P*h*h)/min_EV)))
eta_r_2 = eta_r_op.apply(source - divergence(t_h))
print(np.sqrt(eta_r_2.dofs.vector.l1_norm()))
_ = visualize_function(eta_r_2)
```

```{code-cell}
from dune.xt.functions import gradient

eta_df_op = Operator(grid, RT_0, fv_space)
eta_df_op += LocalElementBilinearFormIndicatorOperator(
    LocalElementIntegralBilinearForm(LocalElementProductIntegrand(inverse(diffusion, order=0))))
eta_df_2 = eta_df_op.apply(diffusion*gradient(u_h) + t_h)
print(eta_df_2.dofs.vector.l1_norm())
_ = visualize_function(eta_df_2)
```


Download the code:
{download}`example__ESV2007_estimates.md`
{nb-download}`example__ESV2007_estimates.ipynb`
