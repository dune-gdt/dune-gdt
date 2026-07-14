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

# Example: mixed-element and prism grids

Most grid managers hold a single geometry type: `YaspGrid` is all cubes, `ALUGrid` is all cubes
*or* all simplices. `UGGrid` is the exception -- it can hold a mesh that mixes cubes and simplices
(and, in 3d, prisms). The `dune-gdt` C++ test suite exercises those element types through the
`make_mixed_grid` / `make_prism_grid` fixtures (`dune/gdt/test/spaces/base.hh`); this example uses
the matching Python factories `dune.xt.grid.make_mixed_grid` / `make_prism_grid`, which build the
same meshes.

`UGGrid` is an optional dependency (the `uggrid` vcpkg feature). The cell below detects whether the
installed wheel was built with it; if not, the demonstration cells further down are skipped with a
note instead of failing, so this page still builds. (The property tests in
`python/gdt/test/test_hypothesis_mixed_grids.py` exercise the same factories whenever the build
provides them.)

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer

import math

import numpy as np

from dune.xt.grid import Dim, visualize_grid

try:
    from dune.xt.grid import make_mixed_grid, make_prism_grid

    HAVE_UGGRID = True
except ImportError:
    HAVE_UGGRID = False
    print(
        "This wheel was built without the 'uggrid' vcpkg feature, so make_mixed_grid /\n"
        "make_prism_grid are not available and the cells below are skipped. Rebuild with the\n"
        "'uggrid' feature enabled (see CMakePresets.json) to run the full example."
    )
```

A small helper classifies each leaf element by its number of corners (in 2d: 3 -> triangle/simplex,
4 -> quadrilateral/cube; in 3d a prism has 6) and counts how many of each geometry type the grid
holds -- this is what makes a mixed grid visibly different from the structured ones:

```{code-cell}
_GEOMETRY_BY_CORNERS = {
    (2, 3): "simplex",
    (2, 4): "cube",
    (3, 4): "simplex",
    (3, 6): "prism",
    (3, 8): "cube",
}


def element_geometry_counts(grid):
    dim = grid.dimension
    counts = {}

    def visit(element):
        geometry_type = _GEOMETRY_BY_CORNERS.get((dim, element.corners.shape[0]), "unknown")
        counts[geometry_type] = counts.get(geometry_type, 0) + 1

    grid.apply_on_each_element(visit)
    return counts
```

## 1: a mixed cube/simplex grid in 2d

`make_mixed_grid(Dim(2))` inserts two quadrilaterals and two triangles that together tile a small
polygonal domain:

```{code-cell}
if HAVE_UGGRID:
    grid = make_mixed_grid(Dim(2))
    print(f"{grid.size(0)} elements, {grid.size(grid.dimension)} vertices")
    print(element_geometry_counts(grid))
```

We can look at the mesh directly; the two triangles and two quadrilaterals share a common edge
skeleton:

```{code-cell}
if HAVE_UGGRID:
    _ = visualize_grid(grid)
```

## 2: assembling and solving on the mixed grid

A discontinuous Galerkin (interior penalty) discretization is the natural choice here: it assembles
element by element and couples neighbours across intersections, so it does not care whether two
adjacent elements are the same geometry type. We refine the grid a couple of times for a finer
solution -- refinement multiplies the element counts but keeps both geometry types present:

```{code-cell}
if HAVE_UGGRID:
    grid.global_refine(2)
    print(element_geometry_counts(grid))
```

```{code-cell}
if HAVE_UGGRID:
    from discretize_elliptic_ipdg import discretize_elliptic_ipdg_dirichlet_zero

    u_h = discretize_elliptic_ipdg_dirichlet_zero(grid, diffusion=1, source=1)
    _ = visualize_grid(grid)
```

```{code-cell}
if HAVE_UGGRID:
    from dune.gdt import visualize_function

    _ = visualize_function(u_h)
```

The size of the DG space is bookkept per element and per geometry type: an order-`k` cube
contributes `(k + 1)^d` (tensor-product `Q_k`) local degrees of freedom, a simplex contributes
`binomial(k + d, d)` (`P_k`). Summing over the elements actually present must reproduce the space's
global DoF count exactly -- the mixed-grid generalization of the per-element DG DoF property tested
in `python/gdt/test/test_hypothesis_mixed_grids.py`:

```{code-cell}
def dg_dofs_per_element(geometry_type, order, dim):
    if geometry_type == "cube":
        return (order + 1) ** dim  # Q_k
    if geometry_type == "simplex":
        return math.comb(order + dim, dim)  # P_k
    raise NotImplementedError(geometry_type)


if HAVE_UGGRID:
    from dune.gdt import DiscontinuousLagrangeSpace

    order = 1
    space = DiscontinuousLagrangeSpace(grid, order=order)
    expected = sum(
        count * dg_dofs_per_element(geometry_type, order, grid.dimension)
        for geometry_type, count in element_geometry_counts(grid).items()
    )
    print(f"space.num_DoFs = {space.num_DoFs}, expected = {expected}")
    assert space.num_DoFs == expected
```

## 3: a prism grid in 3d

In three dimensions `UGGrid` additionally supports prisms (a triangle extruded along a line).
`make_prism_grid(Dim(3))` builds a single prism, which we refine once to obtain several:

```{code-cell}
if HAVE_UGGRID:
    prism_grid = make_prism_grid(Dim(3), num_refinements=1)
    print(element_geometry_counts(prism_grid))
```

All elements are prisms (6 corners each), a geometry type that is unreachable from Python with any
of the other bound grid managers.

Download the code:
{download}`example__mixed_and_prism_grids.md`
{nb-download}`example__mixed_and_prism_grids.ipynb`
