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

# Example: the ALU cube grid (3d hexahedra)

Historically the Python bindings instantiated exactly *one cube and one simplex grid per
dimension* -- the structured `YaspGrid` for cubes and the conforming `ALUGrid` for simplices.
As part of [#320](https://github.com/dune-gdt/dune-gdt/issues/320) the bindings add the
**unstructured, nonconforming ALU cube grid in 3d** (hexahedra), reached through a
`Nonconforming()` selector:

```python
make_cube_grid(Dim(3), Cube())                   # structured YaspGrid (default)
make_cube_grid(Dim(3), Cube(), Nonconforming())  # unstructured ALU hexahedral grid
```

Existing `make_cube_grid` calls are unchanged; `pybind11` dispatches on the extra tag.

```{admonition} Why only the 3d cube?
:class: note

The bindings register spaces, operators and boundary infos keyed on the grid's *leaf view*
type, and dune-alugrid expresses that view through the legacy `ALU3dGrid<tetra|hexa>` /
`ALU2dGrid` -- the conforming/nonconforming refinement policy is **not part of the view type**.
So the conforming and nonconforming ALU *simplex* grids share one leaf-view type and cannot both
be registered, and there is no distinct 2d cube ALUGrid at all. Only the 3d hexahedral grid has a
leaf view (`ALU3dGrid<hexa>`) distinct from the tetrahedral simplex and structured YASP views, so
it is the one new grid this work package can expose. Closing the nonconforming-*simplex* gap needs
a binding-layer change (keying on the grid rather than the view) and is tracked in #320.
```

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer

from dune.xt.grid import Dim, Cube, Nonconforming, make_cube_grid, visualize_grid
```

## Structured YASP vs. unstructured ALU cube

Both grids start from the same structured `2 x 2 x 2` box, so the macro grids agree on element and
vertex counts:

```{code-cell}
domain = dict(lower_left=[0, 0, 0], upper_right=[1, 1, 1], num_elements=[2, 2, 2])

yasp = make_cube_grid(Dim(3), Cube(), **domain)
alu = make_cube_grid(Dim(3), Cube(), Nonconforming(), **domain)

for name, grid in (("YASP (structured)", yasp), ("ALU (unstructured)", alu)):
    print(f"{name:20s}: {grid.size(0)} elements, {grid.size(3)} vertices")
```

One global refinement splits every hexahedron into `2^d = 8` children, for both implementations:

```{code-cell}
for name, grid in (("YASP (structured)", yasp), ("ALU (unstructured)", alu)):
    before = grid.size(0)
    grid.global_refine(1)
    print(f"{name:20s}: {before} -> {grid.size(0)} elements  (x{grid.size(0) // before})")
```

## A finite-element space on the ALU cube grid

The FEM stack works on the new grid like on any other: we can build a `ContinuousLagrangeSpace`,
a `DiscreteFunction`, and visualise the hexahedral grid.

```{code-cell}
from dune.gdt import ContinuousLagrangeSpace, DiscreteFunction

grid = make_cube_grid(Dim(3), Cube(), Nonconforming(),
                      lower_left=[0, 0, 0], upper_right=[1, 1, 1], num_elements=[4, 4, 4])
V_h = ContinuousLagrangeSpace(grid, order=1)
u_h = DiscreteFunction(V_h, name="u_h")

print(f"ALU cube grid: {grid.size(0)} elements, {V_h.num_DoFs} DoFs (Q1)")
_ = visualize_grid(grid)
```

```{admonition} Local adaptation is simplex-only for now
:class: note

The ALU cube grid *supports* local, nonconforming refinement in principle, but `AdaptationHelper`
(and the marking/adapt machinery) is currently bound only for the simplex grids -- see the
{doc}`grid adaptation example <example__simple_grid_adaptation>`, which marks and adapts a 2d
simplex grid. Extending the adaptation bindings to cube grids is a separate binding gap, tracked
in [#320](https://github.com/dune-gdt/dune-gdt/issues/320).
```

## How this reaches the property-test suite

The `GridProvider*` classes that `dune.xt.grid` exposes encode every bound grid combination as
`GridProvider<dim>d<Element><Impl>`. The shared `dune.xt.test.hypothesis_strategies` module
*discovers* the bound grids by parsing exactly these names (see
[#320](https://github.com/dune-gdt/dune-gdt/issues/320) WP0), so the new `GridProvider3dCubeAlunonconformgrid`
is drawn by the property tests automatically -- no strategy edit was needed to start exercising it:

```{code-cell}
import dune.xt.grid as xtgrid

for name in sorted(n for n in dir(xtgrid) if n.startswith("GridProvider")):
    print(name)
```
