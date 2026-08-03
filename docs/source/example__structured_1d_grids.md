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

# Example: structured 1d grids (`YASP_1D` vs `OneDGrid`)

Until now the Python bindings exposed exactly one one-dimensional grid, `Dune::OneDGrid`
(`make_cube_grid(Dim(1), ...)`). WP2 of
[issue #320](https://github.com/dune-gdt/dune-gdt/issues/320) adds the structured
`YaspGrid<1, EquidistantOffsetCoordinates>` next to it -- the 1d sibling of the `YASP_2D` /
`YASP_3D` cube grids that already back the 2d/3d examples, and the grid the C++ `SimplicialGrids`
typed-test list uses in 1d. This notebook

* builds the **same** 1d domain as both an `OneDGrid` and a `YASP_1D` grid,
* solves the stationary heat equation on each and checks the two discretizations agree, and
* demonstrates the *equidistant-offset* feature (a structured grid on an arbitrary interval
  `[a, b]`, `a != 0`) that motivates having a structured 1d grid at all.

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer


import numpy as np
```

## Two ways to mesh the unit interval

`OneDGrid` is selected through the dimension-only `make_cube_grid` overload, while the structured
`YASP_1D` grid is selected through the same `(Dimension, Cube)` overload as its 2d/3d counterparts
-- so both one-dimensional grids can coexist without an ambiguous factory signature.

```{code-cell}
from dune.xt.grid import Cube, Dim, make_cube_grid

n = 8  # number of intervals
lower_left, upper_right = [0.0], [1.0]

oned_grid = make_cube_grid(Dim(1), lower_left=lower_left, upper_right=upper_right,
                           num_elements=[n])
yasp_grid = make_cube_grid(Dim(1), Cube(), lower_left=lower_left, upper_right=upper_right,
                           num_elements=[n])

for name, grid in (("OneDGrid", oned_grid), ("YASP_1D", yasp_grid)):
    print(f'{name:9s}: {grid.size(0)} elements, {grid.size(1)} vertices')
```

Both provide the identical structured skeleton (`num_elements` equal intervals of width
`h = 1/num_elements`); the element centers coincide up to ordering:

```{code-cell}
oned_centers = np.sort(np.array(oned_grid.centers(0), copy=False).ravel())
yasp_centers = np.sort(np.array(yasp_grid.centers(0), copy=False).ravel())

assert np.allclose(oned_centers, yasp_centers)
print('element centers:', yasp_centers)
```

## The stationary heat equation on both grids

We reuse the continuous-Lagrange discretization from `discretize_elliptic_cg.py` (the same helper
the 2d CG tutorial uses) to solve

$$-u''(x) = 1 \quad\text{on } (0, 1), \qquad u(0) = u(1) = 0,$$

whose exact solution is $u(x) = \tfrac{1}{2}\,x\,(1 - x)$.

```{code-cell}
from discretize_elliptic_cg import discretize_elliptic_cg_dirichlet_zero

def solve(grid):
    # constant unit diffusion and unit source, homogeneous Dirichlet everywhere
    u_h = discretize_elliptic_cg_dirichlet_zero(grid, diffusion=1.0, source=1.0)
    return np.array(u_h.dofs.vector, copy=False)

oned_dofs = solve(oned_grid)
yasp_dofs = solve(yasp_grid)
```

The two grids give rise to the identical P1 linear system (up to a DoF permutation), so the sorted
DoF vectors must match to solver accuracy:

```{code-cell}
assert oned_dofs.shape == yasp_dofs.shape == (n + 1,)
grid_to_grid = np.max(np.abs(np.sort(oned_dofs) - np.sort(yasp_dofs)))
print(f'max |OneDGrid - YASP_1D| over the DoFs: {grid_to_grid:.2e}')
assert grid_to_grid < 1e-6
```

In one dimension the P1 Galerkin solution of the Poisson problem is *nodally exact*, so the sorted
DoF values reproduce the exact solution sampled on the nodes $x_i = i/N$:

```{code-cell}
nodes = np.linspace(0.0, 1.0, n + 1)
exact_nodal = np.sort(0.5 * nodes * (1.0 - nodes))
nodal_error = np.max(np.abs(np.sort(yasp_dofs) - exact_nodal))
print(f'max nodal error vs. exact solution: {nodal_error:.2e}')
assert nodal_error < 1e-6
```

## The equidistant-offset feature: a structured grid on `[a, b]`

The whole point of `EquidistantOffsetCoordinates` is that the structured grid need not start at the
origin. We put a `YASP_1D` grid on the offset interval `[2, 5]` and solve the same problem there,

$$-u''(x) = 1 \quad\text{on } (2, 5), \qquad u(2) = u(5) = 0,$$

with exact solution $u(x) = \tfrac{1}{2}\,(x - 2)\,(5 - x)$.

```{code-cell}
a, b = 2.0, 5.0
offset_grid = make_cube_grid(Dim(1), Cube(), lower_left=[a], upper_right=[b],
                             num_elements=[n])

centers = np.sort(np.array(offset_grid.centers(0), copy=False).ravel())
h = (b - a) / n
print(f'offset grid spans [{centers.min() - 0.5 * h:.1f}, {centers.max() + 0.5 * h:.1f}]')

offset_dofs = solve(offset_grid)
offset_nodes = np.linspace(a, b, n + 1)
offset_exact = np.sort(0.5 * (offset_nodes - a) * (b - offset_nodes))
offset_error = np.max(np.abs(np.sort(offset_dofs) - offset_exact))
print(f'max nodal error on [{a}, {b}]: {offset_error:.2e}')
assert offset_error < 1e-6
```

The structured 1d grid solves the shifted problem exactly, without any special handling of the
non-zero offset -- the same behaviour the 2d/3d `YASP` cube grids already provide, now available in
one dimension for property testing and structured-grid experiments alongside `OneDGrid`.

Download the code:
{download}`example__structured_1d_grids.md`
{nb-download}`example__structured_1d_grids.ipynb`
