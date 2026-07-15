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

# Example: Taylor-Hood P2/P1 Stokes

The C++ test suite (`dune/gdt/test/stokes/stokes-taylorhood.hh`) solves a stationary Stokes problem with
a Taylor-Hood discretization: a **vector-valued**, order-2 continuous Lagrange space for the velocity
paired with a scalar, order-1 continuous Lagrange space for the pressure. Until now this was impossible
from Python: the bindings only ever instantiated `ContinuousLagrangeSpace` for scalar
(`dim_range=Dim(1)`) fields, even though `DiscontinuousLagrangeSpace` and `FiniteVolumeSpace` already had
the `dim_range=Dim(d)` overload (#320, WP4). This example is the first Python reproduction of that
reference test.

We solve
$$-\Delta u + \nabla p = f, \qquad \operatorname{div} u = g$$
on $\Omega = (-1, 1)^2$ with Dirichlet boundary values everywhere, using the manufactured "testcase 1"
solution from `dune/gdt/data/stokes.hh` ($f = 0$, $g = 0$, and $u$, $p$ given below), so we can compare
the discrete solution against the exact one.

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer

import numpy as np
```

## grid and spaces

```{code-cell}
from dune.xt.grid import AllDirichletBoundaryInfo, Dim, Simplex, Walker, make_cube_grid

d = 2
grid = make_cube_grid(Dim(d), Simplex(), [-1, -1], [1, 1], [6, 6])
boundary_info = AllDirichletBoundaryInfo(grid)

print(f'grid has {grid.size(0)} elements, {grid.size(d)} vertices')
```

```{code-cell}
from dune.gdt import ContinuousLagrangeSpace

# the vector-valued factory overload added for WP4: dim_range=Dim(d) selects the r=d instantiation,
# exactly the way DiscontinuousLagrangeSpace/FiniteVolumeSpace already worked
velocity_space = ContinuousLagrangeSpace(grid, order=2, dim_range=Dim(d))
pressure_space = ContinuousLagrangeSpace(grid, order=1)

print(f'velocity_space: {velocity_space.num_DoFs} DoFs (order 2, {d}-valued)')
print(f'pressure_space: {pressure_space.num_DoFs} DoFs (order 1, scalar)')
```

Two related contract limits of `ContinuousLagrangeSpace` are documented (not lifted) as part of WP4:
order $> 2$ on non-cube grids is a hard mapper limitation (`dune/gdt/spaces/mapper/continuous.hh`), and
`RaviartThomasSpace` only supports order $0$ (`dune/gdt/spaces/hdiv/raviart-thomas.hh`) -- neither is hit
here, since Taylor-Hood only ever needs order $\le 2$ CG spaces.

## the exact solution

```{code-cell}
from dune.xt.functions import ExpressionFunction, GridFunction as GF

u_exact_expr = ExpressionFunction(
    dim_domain=Dim(d),
    variable='x',
    expressions=[
        '-1*exp(x[0])*(x[1]*cos(x[1])+sin(x[1]))',
        'exp(x[0])*x[1]*sin(x[1])',
    ],
    order=5,
    name='u_exact',
)
p_exact_expr = ExpressionFunction(
    dim_domain=Dim(d),
    variable='x',
    expression='2*exp(x[0])*sin(x[1])',
    order=5,
    name='p_exact',
)
```

## assembling $A$ (velocity Laplacian) and $B$ (pressure/velocity divergence coupling)

The saddle-point system is
$$\begin{pmatrix} A & B \\ B^T & 0 \end{pmatrix} \begin{pmatrix} u \\ p \end{pmatrix}
  = \begin{pmatrix} f \\ g \end{pmatrix}, \qquad
  A_{ij} = \int_\Omega \nabla v_i : \nabla v_j, \qquad
  B_{ij} = -\int_\Omega p_j \operatorname{div}(v_i).$$

$B$ pairs a *vector* test basis (velocity, rank $d$) with a *scalar* ansatz basis (pressure, rank $1$):
this rectangular combination needed two more WP4 fixes to reach from Python -- a new
`LocalElementAnsatzValueTestDivProductIntegrand` binding (the C++ class existed, just unbound), and a
`BilinearForm(grid, ansatz_range=..., test_range=...)` disambiguation, since the previously grid-only
factory could not otherwise tell its four rank instantiations apart.

```{code-cell}
from dune.gdt import (
    BilinearForm,
    LocalElementAnsatzValueTestDivProductIntegrand,
    LocalElementIntegralBilinearForm,
    LocalLaplaceIntegrand,
    MatrixOperator,
)

diffusion = GF(grid, 1.0, dim_range=(Dim(d), Dim(d)), name='diffusion')

a_form = BilinearForm(grid, ansatz_range=Dim(d), test_range=Dim(d))
a_form += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(diffusion))
A = MatrixOperator(grid, source_space=velocity_space, range_space=velocity_space)
A.append(a_form)

b_form = BilinearForm(grid, ansatz_range=Dim(1), test_range=Dim(d))
b_form += LocalElementIntegralBilinearForm(LocalElementAnsatzValueTestDivProductIntegrand(GF(grid, -1.0)))
B = MatrixOperator(grid, source_space=pressure_space, range_space=velocity_space)
B.append(b_form)
```

```{code-cell}
from dune.gdt import DirichletConstraints, boundary_interpolation
from dune.xt.grid import DirichletBoundary

dirichlet_constraints = DirichletConstraints(boundary_info, velocity_space)

walker = Walker(grid)
walker.append(A)
walker.append(B)
walker.append(dirichlet_constraints)
walker.walk()

discrete_dirichlet_values = boundary_interpolation(
    GF(grid, u_exact_expr), velocity_space, boundary_info, DirichletBoundary()
)
```

## solving the saddle-point system

`dune.xt.la` does not (yet) expose a saddle-point solver to Python, so we assemble the small dense
system directly with `get_entry`/numpy: eliminate the Dirichlet velocity DoFs by row replacement, and
pin one pressure DoF to remove the constant pressure null space (the same two steps
`stokes-taylorhood.hh` performs on the C++ containers before calling `XT::LA::SaddlePointSolver`).

```{code-cell}
n_u = velocity_space.num_DoFs
n_p = pressure_space.num_DoFs

A_np = np.array([[A.matrix.get_entry(i, j) for j in range(n_u)] for i in range(n_u)])
B_np = np.array([[B.matrix.get_entry(i, j) for j in range(n_p)] for i in range(n_u)])

g_D = np.array(discrete_dirichlet_values.dofs.vector)

rhs_u = np.zeros(n_u)
rhs_p = np.zeros(n_p)

# B_upper feeds the velocity equations (row i replaced by the trivial u[i] = g_D[i] at a
# Dirichlet DoF, so its pressure coupling is dropped there too); B_lower feeds the pressure
# (divergence) equations and must keep every column's *true* entries -- including at the
# Dirichlet velocity DoFs -- since u there still enters those equations, just as a value fixed
# by A_np's unit rows rather than as a free unknown. Reusing one row-zeroed matrix for both
# blocks would silently drop that div(u)|_{Dirichlet} contribution from the pressure equations.
B_upper = B_np.copy()
B_lower = B_np.T.copy()

for dof in dirichlet_constraints.dirichlet_DoFs:
    A_np[dof, :] = 0.0
    A_np[dof, dof] = 1.0
    B_upper[dof, :] = 0.0
    rhs_u[dof] = g_D[dof]

# pin pressure DoF 0 to p = 0: decouple it from every velocity equation (its true contribution
# is p[0] * B_np[:, 0] = 0 anyway) and make its own row a pure identity, decoupled from velocity
B_upper[:, 0] = 0.0
B_lower[0, :] = 0.0

K = np.zeros((n_u + n_p, n_u + n_p))
K[:n_u, :n_u] = A_np
K[:n_u, n_u:] = B_upper
K[n_u:, :n_u] = B_lower
K[n_u, n_u] = 1.0

rhs = np.concatenate([rhs_u, rhs_p])
solution = np.linalg.solve(K, rhs)
u_dofs, p_dofs = solution[:n_u], solution[n_u:]
```

```{code-cell}
from dune.gdt import DiscreteFunction

u_h = DiscreteFunction(velocity_space, name='u_h')
for ii in range(n_u):
    u_h.dofs.vector[ii] = u_dofs[ii]

p_h = DiscreteFunction(pressure_space, name='p_h')
for ii in range(n_p):
    p_h.dofs.vector[ii] = p_dofs[ii]
```

```{code-cell}
u_h.visualize('example__stokes_taylor_hood_velocity')
p_h.visualize('example__stokes_taylor_hood_pressure')
```

Start `paraview` in a terminal to visualize `u_h`/`p_h`.

## comparing against the exact solution

Interpolating the exact solution into the same spaces gives a cheap, DoF-wise sanity check (a proper
$L^2$ error would need `dune.gdt.l2_norm`, which is not bound to Python):

```{code-cell}
from dune.gdt import default_interpolation

u_exact_h = default_interpolation(GF(grid, u_exact_expr), velocity_space)
p_exact_h = default_interpolation(GF(grid, p_exact_expr), pressure_space)

u_error = np.max(np.abs(np.array(u_h.dofs.vector) - np.array(u_exact_h.dofs.vector)))

# p_h was pinned to p_h[0] = 0, which the exact solution does not satisfy; shift p_exact by its
# value at that same DoF before comparing, since Stokes pressure is only unique up to a constant
p_exact_dofs = np.array(p_exact_h.dofs.vector)
p_error = np.max(np.abs(np.array(p_h.dofs.vector) - (p_exact_dofs - p_exact_dofs[0])))

print(f'max |u_h - u_exact| at the DoFs: {u_error:.3e}')
print(f'max |p_h - p_exact| at the DoFs (up to the additive constant pinned by DoF 0): {p_error:.3e}')
```

Download the code:
{download}`example__stokes_taylor_hood.md`
{nb-download}`example__stokes_taylor_hood.ipynb`
