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

# Example: working with gmsh grids

## 1: creating a gmsh file

We use pyMORs `PolygonalDomain` description and `discretize_gmsh` to obtain a grid file that `gmsh` can read. **Note** that `dune-grid` can only handle `gmsh` version 2 files. Depending on the installed `gmsh` version, the file written below may use a newer format; we convert it to a `dune-grid`-readable version 2.2 mesh with `meshio` further down (see also [these instructions](https://gitlab.dune-project.org/core/dune-grid/issues/85)).

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer


import numpy as np

```

```{code-cell}
from pymor.analyticalproblems.domaindescriptions import PolygonalDomain

L_shaped_domain = PolygonalDomain(points=[
    [0, 0],
    [0, 1],
    [-1, 1],
    [-1, -1],
    [1, -1],
    [1, 0],
], boundary_types={'dirichlet': [1, 2, 3, 4, 5, 6]})
```

```{code-cell}
from pymor.discretizers.builtin.domaindiscretizers.gmsh import discretize_gmsh

grid, bi = discretize_gmsh(L_shaped_domain, msh_file_path='L_shaped_domain.msh')
```

## 2: discretization using pyMOR

We may use pyMOR to solve an elliptic PDE on this grid.

```{code-cell}
from pymor.analyticalproblems.functions import ConstantFunction
from pymor.analyticalproblems.elliptic import StationaryProblem

problem = StationaryProblem(
    domain=L_shaped_domain,
    rhs=ConstantFunction(1, dim_domain=2),
    diffusion=ConstantFunction(1, dim_domain=2),
)
```

```{code-cell}
from pymor.discretizers.builtin import discretize_stationary_cg

fom, fom_data = discretize_stationary_cg(problem, grid=grid, boundary_info=bi)
```

```{code-cell}
fom.visualize(fom.solve())
```

## 3: using the gmsh grid in dune

`dune-grid` [only supports](https://gitlab.dune-project.org/core/dune-grid/issues/85) `gmsh` version 2 files, and only a subset of the specification.
Depending on the installed `gmsh` version, `discretize_gmsh` above may have written a newer (version 4) mesh file, and the file also contains a boundary type (`$PhysicalNames`) definition which `dune-grid` cannot [correctly parse](https://gitlab.dune-project.org/core/dune-grid/-/issues/89) (we do not require it, we have our own boundary info).
We therefore use [`meshio`](https://github.com/nschloe/meshio) to re-write a clean version 2.2 ASCII mesh that contains only the points and the triangle cells (**Note** that you have to provide the same filename here as in the call to `discretize_gmsh`):

```{code-cell}
import meshio

_mesh = meshio.read('L_shaped_domain.msh')
meshio.write_points_cells(
    'L_shaped_domain_dune.msh',
    _mesh.points,
    [('triangle', _mesh.get_cells_type('triangle'))],
    file_format='gmsh22',
    binary=False,
)
```

```{code-cell}
from dune.xt.grid import make_gmsh_grid, Dim, Simplex, visualize_grid

grid = make_gmsh_grid('L_shaped_domain_dune.msh', Dim(2), Simplex())
```

This grid can now be used as any other grid, e.g. for visualization ...

```{code-cell}
_ = visualize_grid(grid)
```

... or discretization:

```{code-cell}
from dune.gdt import visualize_function

from discretize_elliptic_cg import discretize_elliptic_cg_dirichlet_zero

u_h = discretize_elliptic_cg_dirichlet_zero(grid, diffusion=1, source=1)
_ = visualize_function(u_h)
```

Download the code:
{download}`example__gmsh_grid.md`
{nb-download}`example__gmsh_grid.ipynb`
