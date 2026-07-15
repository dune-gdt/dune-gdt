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

# Example: LA eigensolvers

This example bridges the FEM and linear algebra layers (WP5 of #320): we assemble the stiffness
matrix of the 2d Laplace operator with a `MatrixOperator`, hand the plain `dune.xt.la` matrix to
the newly bound `EigenSolver`, and compare the smallest computed eigenvalues against the known
eigenvalues of the Dirichlet-Laplace operator on the unit square.

## assembling the Laplace stiffness matrix

We reuse the `ContinuousLagrangeSpace` + `DirichletConstraints` pattern from the
[CG FEM tutorial](dune_gdt_tutorial_on_cg_fem_for_the_stationary_heat_equation.md): a P1 space on
a structured triangulation of $\Omega = [0, 1]^2$, with the Laplace bilinear form
$a(u, v) = \int_\Omega \nabla u \cdot \nabla v\,\text{d}x$ assembled into a matrix, then
Dirichlet-constrained (each boundary DoF's row becomes a unit row).

```{code-cell}
import numpy as np

from dune.xt.grid import (
    AllDirichletBoundaryInfo,
    Dim,
    Simplex,
    Walker,
    make_cube_grid,
)
from dune.xt.functions import GridFunction as GF
from dune.gdt import (
    BilinearForm,
    ContinuousLagrangeSpace,
    DirichletConstraints,
    LocalElementIntegralBilinearForm,
    LocalLaplaceIntegrand,
    MatrixOperator,
    make_element_sparsity_pattern,
)

d = 2
grid = make_cube_grid(Dim(d), Simplex(), lower_left=[0, 0], upper_right=[1, 1], num_elements=[8, 8])
boundary_info = AllDirichletBoundaryInfo(grid)

V_h = ContinuousLagrangeSpace(grid, order=1)
print(f'V_h has {V_h.num_DoFs} DoFs')

a_form = BilinearForm(grid)
a_form += LocalElementIntegralBilinearForm(LocalLaplaceIntegrand(GF(grid, 1., dim_range=(Dim(d), Dim(d)))))
a_h = MatrixOperator(grid, source_space=V_h, range_space=V_h,
                     sparsity_pattern=make_element_sparsity_pattern(V_h))
a_h.append(a_form)

dirichlet_constraints = DirichletConstraints(boundary_info, V_h)

walker = Walker(grid)
walker.append(a_h)
walker.append(dirichlet_constraints)
walker.walk()

dirichlet_constraints.apply(a_h.matrix)

print(f'assembled a {a_h.matrix.rows}x{a_h.matrix.cols} {type(a_h.matrix).__name__}')
```

## computing eigenvalues via the bound eigen-solver

`dune.xt.la` binds one `<Matrix class name>EigenSolver` per matrix type the C++ `.tpl` eigen-solver
test suite already covers (`dune/xt/test/la/eigensolver_for_*.py`), `CommonDenseMatrix` among them.
`MatrixOperator` assembles into whatever the (build-dependent) default LA backend is, so -- to keep
this example runnable on any build -- we convert the assembled matrix to `CommonDenseMatrix`, which
`dune.xt.la` always binds an `EigenSolver` for.

```{code-cell}
import dune.xt.la as la

n = a_h.matrix.rows
assembled_backend = type(a_h.matrix).__name__
print(f'MatrixOperator assembled into a {assembled_backend} ({n}x{n})')

dense_matrix = la.CommonDenseMatrix(n, n, 0.)
for ii in range(n):
    for jj in range(n):
        dense_matrix.set_entry(ii, jj, a_h.matrix.get_entry(ii, jj))

solver = la.CommonDenseMatrixEigenSolver(dense_matrix)
print(f'CommonDenseMatrixEigenSolver.types() = {la.CommonDenseMatrixEigenSolver.types()}')

eigenvalues = np.array([ev.real for ev in solver.eigenvalues()])
```

`dirichlet_constraints.apply` turned every boundary DoF's row into a unit row, so the assembled
matrix has an eigenvalue of exactly `1` once per boundary DoF -- those are an artifact of how the
Dirichlet condition is imposed algebraically, not part of the (interior) Dirichlet-Laplace
spectrum. We filter them out before comparing:

```{code-cell}
interior_eigenvalues = np.sort(eigenvalues[np.abs(eigenvalues - 1.) > 1e-8])
print(f'{len(eigenvalues) - len(interior_eigenvalues)} boundary DoFs (eigenvalue 1, filtered out)')
print(f'smallest interior eigenvalues: {interior_eigenvalues[:6]}')
```

## comparison with the known spectrum of the unit square

The eigenvalues of the Dirichlet-Laplace operator on $\Omega = [0, 1]^2$ are known analytically:
$\lambda_{m, n} = \pi^2 (m^2 + n^2)$ for $m, n = 1, 2, 3, \dots$. On a moderately refined P1 grid
the discrete stiffness matrix reproduces the smallest ones reasonably well (finite element
discretization systematically *overestimates* the true eigenvalues, with the relative error
growing for higher modes -- so we only expect the first few to be close).

```{code-cell}
import itertools

analytic = sorted(
    np.pi ** 2 * (m ** 2 + n ** 2) for m, n in itertools.product(range(1, 4), repeat=2)
)[:6]
print(f'analytic:  {np.round(analytic, 2)}')
print(f'numerical: {np.round(interior_eigenvalues[:6], 2)}')

relative_error = np.abs(interior_eigenvalues[:6] - analytic) / analytic
print(f'relative error: {np.round(relative_error, 3)}')
assert relative_error[0] < 0.1, 'the fundamental mode should already be within 10% on this grid'
```

## matrix-inverter cross-check

As a sanity check on the newly bound `MatrixInverter` (dune/xt/la/matrix-inverter.hh), we verify
that inverting the (non-singular, Dirichlet-constrained) stiffness matrix and applying it to a
column of the inverse recovers the corresponding unit vector, `M @ M_inv[:, j] == e_j`:

```{code-cell}
inverse = la.CommonDenseMatrixMatrixInverter(dense_matrix).inverse()
print(f'CommonDenseMatrixMatrixInverter.types() = {la.CommonDenseMatrixMatrixInverter.types()}')

max_residual = 0.
for j in (0, n // 2, n - 1):
    column = la.CommonVector(n, 0.)
    for ii in range(n):
        column.set_entry(ii, inverse.get_entry(ii, j))
    unit_vector = dense_matrix.dot(column)
    residual = np.array([unit_vector.get_entry(ii) for ii in range(n)])
    residual[j] -= 1.
    max_residual = max(max_residual, np.abs(residual).max())

print(f'max |M @ M_inv[:, j] - e_j| over the sampled columns: {max_residual:.2e}')
assert max_residual < 1e-8
```

Download the code:
{download}`example__la_eigensolvers.md`
{nb-download}`example__la_eigensolvers.ipynb`
