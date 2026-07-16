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
and mass matrices of the 2d Laplace operator with a `MatrixOperator`, reduce the resulting
generalized eigenvalue problem to a standard one using the newly bound `MatrixInverter`, hand the
plain `dune.xt.la` matrix to the newly bound `EigenSolver`, and compare the smallest computed
eigenvalues against the known eigenvalues of the Dirichlet-Laplace operator on the unit square.

## assembling the Laplace stiffness and mass matrices

We reuse the `ContinuousLagrangeSpace` + `DirichletConstraints` pattern from the
[CG FEM tutorial](dune_gdt_tutorial_on_cg_fem_for_the_stationary_heat_equation.md): a P1 space on
a structured triangulation of $\Omega = [0, 1]^2$. The Galerkin FEM discretization of the
Dirichlet-Laplace eigenvalue problem $-\Delta u = \lambda u$ is the *generalized* matrix eigenvalue
problem $K u = \lambda M u$, not the eigenvalue problem of the stiffness matrix $K$ alone -- so we
assemble both the Laplace bilinear form $a(u, v) = \int_\Omega \nabla u \cdot \nabla v\,\text{d}x$
(giving $K$) and the $L^2$ product $m(u, v) = \int_\Omega u \, v\,\text{d}x$ (giving $M$), then
Dirichlet-constrain $K$ (each boundary DoF's row becomes a unit row).

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
    LocalElementProductIntegrand,
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

m_form = BilinearForm(grid)
m_form += LocalElementIntegralBilinearForm(LocalElementProductIntegrand(GF(grid, 1.)))
m_h = MatrixOperator(grid, source_space=V_h, range_space=V_h,
                     sparsity_pattern=make_element_sparsity_pattern(V_h))
m_h.append(m_form)

dirichlet_constraints = DirichletConstraints(boundary_info, V_h)

walker = Walker(grid)
walker.append(a_h)
walker.append(m_h)
walker.append(dirichlet_constraints)
walker.walk()

dirichlet_constraints.apply(a_h.matrix)

print(f'assembled K, M as {a_h.matrix.rows}x{a_h.matrix.cols} {type(a_h.matrix).__name__}')
```

## restricting to the interior DoFs

`dirichlet_constraints.apply` turned every boundary DoF's row and column of $K$ into a unit
row/column (`ensure_symmetry=True`, the default), so $K$'s eigenvalues (of the generalized problem
with $M$) are the interior Dirichlet-Laplace spectrum we want *plus* an extra eigenvalue of exactly
`1` for every boundary DoF -- an artifact of how the Dirichlet condition is imposed algebraically,
not part of the Dirichlet-Laplace spectrum itself. Rather than filter those out after the fact, we
discard the boundary rows/columns of both $K$ and $M$ before solving: `dirichlet_constraints.dirichlet_DoFs`
gives the boundary DoF indices, and since `apply` never touched $K$'s interior-interior entries (and
we never constrained $M$ to begin with), the interior/interior submatrices are exactly the discrete
operators for the interior eigenvalue problem.

```{code-cell}
n = a_h.matrix.rows
boundary_dofs = set(dirichlet_constraints.dirichlet_DoFs)
interior_dofs = [dof for dof in range(n) if dof not in boundary_dofs]
print(f'{n} DoFs total, {len(boundary_dofs)} on the boundary, {len(interior_dofs)} interior')
```

## reducing to a standard eigenvalue problem

`dune.xt.la` binds one `<Matrix class name>EigenSolver` per matrix type the C++ `.tpl` eigen-solver
test suite already covers (`dune/xt/test/la/eigensolver_for_*.py`), `CommonDenseMatrix` among them.
`MatrixOperator` assembles into whatever the (build-dependent) default LA backend is, so -- to keep
this example runnable on any build -- we convert the interior/interior submatrices of $K$ and $M$ to
`CommonDenseMatrix`, which `dune.xt.la` always binds an `EigenSolver` for.

`GeneralizedEigenSolverOptions` (dune/xt/la/generalized-eigen-solver/default.hh) only offers a
LAPACK-backed `"lapack"` type, with no LAPACK-independent fallback (unlike `EigenSolverOptions`,
which falls back to a pure-C++ `"shifted_qr"` implementation) -- a newly-discovered gap, only
visible now that this WP binds the generalized eigensolver at all; see the accompanying PR for
details. Since this example needs to run on any build, including ones without LAPACK, we instead
reduce $K u = \lambda M u$ to the standard problem $A u = \lambda u$ with $A = M^{-1} K$ ourselves,
using the newly bound `MatrixInverter` to invert $M$ (SPD for any conforming FE space, hence
invertible) -- putting both of this WP's new solver bindings to use for one coherent example.

```{code-cell}
import dune.xt.la as la

n_interior = len(interior_dofs)
assembled_backend = type(a_h.matrix).__name__
print(f'MatrixOperator assembled into a {assembled_backend}, restricted to a {n_interior}x{n_interior} interior system')

def interior_submatrix(matrix):
    sub = la.CommonDenseMatrix(n_interior, n_interior, 0.)
    for ii, gi in enumerate(interior_dofs):
        for jj, gj in enumerate(interior_dofs):
            sub.set_entry(ii, jj, matrix.get_entry(gi, gj))
    return sub

def to_numpy(matrix):
    return np.array([[matrix.get_entry(ii, jj) for jj in range(n_interior)] for ii in range(n_interior)])

dense_matrix = interior_submatrix(a_h.matrix)  # K, restricted to the interior DoFs
dense_mass_matrix = interior_submatrix(m_h.matrix)  # M, restricted to the interior DoFs

mass_inverse = la.CommonDenseMatrixMatrixInverter(dense_mass_matrix).inverse()
print(f'CommonDenseMatrixMatrixInverter.types() = {la.CommonDenseMatrixMatrixInverter.types()}')

reduced_matrix_np = to_numpy(mass_inverse) @ to_numpy(dense_matrix)  # A = M^{-1} K
reduced_matrix = la.CommonDenseMatrix(n_interior, n_interior, 0.)
for ii in range(n_interior):
    for jj in range(n_interior):
        reduced_matrix.set_entry(ii, jj, reduced_matrix_np[ii, jj])
```

## computing eigenvalues via the bound eigen-solver

We only need eigenvalues here, so we explicitly disable eigenvector computation. We also pin the
solver type to `"shifted_qr"` rather than leaving it at the default (which picks `"lapack"`
whenever LAPACK is available): two newly-discovered, previously-latent defects in the LAPACK-backed
code path only reachable now that this WP binds the eigensolver at all -- crashes in both the
eigenvalues-*and*-eigenvectors branch and, separately, the eigenvalues-*only* branch -- mean the
`"lapack"` type is currently unsafe to use with `compute_eigenvectors=false`. `"shifted_qr"` has
neither issue and is thoroughly covered by the existing C++ test suite; see the accompanying PR for
details.

```{code-cell}
eigensolver_opts = dict(la.CommonDenseMatrixEigenSolver.options('shifted_qr'))
eigensolver_opts['compute_eigenvectors'] = 'false'
solver = la.CommonDenseMatrixEigenSolver(reduced_matrix, eigensolver_opts)
print(f'CommonDenseMatrixEigenSolver.types() = {la.CommonDenseMatrixEigenSolver.types()}')

interior_eigenvalues = np.sort(np.array([ev.real for ev in solver.eigenvalues()]))
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
that inverting the (non-singular) interior stiffness matrix and applying it to a column of the
inverse recovers the corresponding unit vector, `K @ K_inv[:, j] == e_j`:

```{code-cell}
inverse = la.CommonDenseMatrixMatrixInverter(dense_matrix).inverse()
print(f'CommonDenseMatrixMatrixInverter.types() = {la.CommonDenseMatrixMatrixInverter.types()}')

max_residual = 0.
for j in (0, n_interior // 2, n_interior - 1):
    column = la.CommonVector(n_interior, 0.)
    for ii in range(n_interior):
        column.set_entry(ii, inverse.get_entry(ii, j))
    unit_vector = dense_matrix.dot(column)
    residual = np.array([unit_vector.get_entry(ii) for ii in range(n_interior)])
    residual[j] -= 1.
    max_residual = max(max_residual, np.abs(residual).max())

print(f'max |K @ K_inv[:, j] - e_j| over the sampled columns: {max_residual:.2e}')
assert max_residual < 1e-8
```

Download the code:
{download}`example__la_eigensolvers.md`
{nb-download}`example__la_eigensolvers.ipynb`
