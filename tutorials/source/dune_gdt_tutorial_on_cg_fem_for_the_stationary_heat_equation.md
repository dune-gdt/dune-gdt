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

# Tutorial: continuous FEM for the stationary heat equation

This tutorial shows how to solve the stationary heat equation using continuous Finite Elements with `dune-gdt`.

This is work in progress [WIP], still missing:

* Neumann boundary values
* Robin boundary values

```{code-cell}
%matplotlib notebook

import numpy as np
np.warnings.filterwarnings('ignore') # silence numpys warnings
```

## diffusion with homogeneous Dirichlet boundary condition

### analytical problem

Let $\Omega \subset \mathbb{R}^d$ for $1 \leq d \leq 3$ be a bounded connected domain with Lipschitz-boundary $\partial\Omega$. We seek the solution $u \in H^1_0(\Omega)$ of the **linear diffusion equation** (with a homogeneous Dirichlet boundary condition)

$$\begin{align}
- \nabla\cdot(\kappa\nabla u) &= f &&\text{in } \Omega,\tag{1}\label{pde}\\
u &= 0 &&\text{on } \partial\Omega,
\end{align}$$

in a weak sense, where $\kappa \in [L^\infty(\Omega)]^{d \times d}$ denotes a given diffusion function and $f \in L^2(\Omega)$ denotes a given source function.

The variational problem associated with $\eqref{pde}$ reads: find $u \in H^1_0(\Omega)$, such that

$$\begin{align}
a(u, v) &= l(v) &&\text{for all }v \in H^1_0(\Omega),\tag{2}\label{weak_formulation}
\end{align}$$

where the bilinear form $a: H^1(\Omega) \times H^1(\Omega) \to \mathbb{R}$ and the linear functional $l \in H^{-1}(\Omega)$ are given by

$$\begin{align}
a(u, v) := \int_\Omega (\kappa\nabla u)\cdot v \,\text{d}x &&\text{and}&& l(v) := \int_\Omega f\,\,\text{d}x,\tag{3}\label{a_and_l}
\end{align}$$

respectively.


Consider for example $\eqref{pde}$ with:

* $d = 2$
* $\Omega = [0, 1]^2$
* $\kappa = 1$
* $f = \exp(x_0 x_1)$

```{code-cell}
from dune.xt.grid import Dim
from dune.xt.functions import ConstantFunction, ExpressionFunction

d = 2
omega = ([0, 0], [1, 1])

kappa = ConstantFunction(dim_domain=Dim(d), dim_range=Dim(1), value=[1.], name='kappa')
# note that we need to prescribe the approximation order,
# which determines the quadrature on each element
f = ExpressionFunction(
    dim_domain=Dim(d), variable='x', expression='exp(x[0]*x[1])', order=3, name='f')
```

### continuous Finite Elements

Let us for simplicity consider a simplicial **grid** $\mathcal{T}_h$ (other types of elements work analogously) as a partition of $\Omega$ into elements $K \in \mathcal{T}_h$ with $h := \max_{K \in \mathcal{T}_h} \text{diam}(K)$, we consider the discrete space of continuous piecewise polynomial functions of order $k \in \mathbb{N}$,

$$\begin{align}
V_h := \big\{ v \in C^0(\Omega) \;\big|\; v|_K \in \mathbb{P}^k(K) \big\}
\end{align}$$

where $\mathbb{P}^k(K)$ denotes the space of polynomials of (total) degree up to $k$ (*note that $V_h \subset H^1(\Omega)$ and $V_h$ does not include the Dirichlet boundary condition, thus $V_h \not\subset H^1_0(\Omega$.*). We obtain a finite-dimensional variational problem by Galerkin-projection of $\eqref{weak_formulation}$ onto $V_h$, thas is: we seek the approximate solution $u_h \in V_h \cap H^1_0(\Omega)$, such that

$$\begin{align}
a(u_h, v_h) &= l(v_h) &&\text{for all }v_h \in V_h \cap H^1_0(\Omega).\tag{4}\label{discrete_variational_problem}
\end{align}$$

A basis of $V_h$ is given by the Lagrangian basis functions

$$\begin{align}
\varPhi := \big\{\varphi_1, \dots, \varphi_N\big\}
\end{align}$$

of order $k$ (e.g., the usual hat-functions for $k = 1$, which we consider from here on), with $N := \text{dim}(V_h)$. As usual, each of these *global* basis functions, if restricted to a grid element, is given by the concatenation of a *local* shape function and the reference map: given

* the invertible affine **reference map** $F_K: \hat{K} \to K$ for all $K \in \mathcal{T}_h$, such that $\hat{x} \mapsto x := F_K(\hat{x})$, where $\hat{K}$ is the reference element associated with $K$,
* a set of Lagrangian **shape functions** $\{\hat{\varphi}_1, \dots, \hat{\varphi}_{d + 1}\} \in \mathbb{P}^1(K)$, each associated with a vertex $\hat{a}_1, \dots, \hat{a}_{d + 1}$ of the reference element $\hat{K}$ and
* a **DoF mapping** $\sigma_K: \{1, \dots, d + 1\} \to \{1, \dots, N\}$ for all $K \in \mathcal{T}_h$, which associates the *local* index $\hat{i} \in \{1, \dots, d + 1\}$ of a vertex $\hat{a}_{\hat{i}}$ of the reference element $\hat{K}$ with the *global* index $i \in \{1, \dots, N\}$ of the vertex $a_i := F_K(\hat{a}_\hat{i})$ in the grid $\mathcal{T}_h$.

The DoF mapping as well as a localizable global basis is provided by a **discrete function space** in `dune-gdt`.
We thus have

$$\begin{align}
\varphi_i|_K &= \hat{\varphi}_\hat{i}\circ F_K^{-1} &&\text{and}\tag{5}\label{basis_transformation}\\
(\nabla\varphi_i)|_K &= \nabla\big(\hat{\varphi}_\hat{i}\circ F_K^{-1}\big) = \nabla F_K^{-1} \cdot \big(\nabla\hat{\varphi}_\hat{i}\circ F_K^{-1}\big),\tag{6}\label{basis_transformation_grad}
\end{align}$$

owing to the chain rule, with $i := \sigma_K(\hat{i})$ for all $1 \leq \hat{i} \leq d+1$ and all $K \in \mathcal{T}_h$.

To obtain the algebraic analogue to $\eqref{discrete_variational_problem}$, we

* replace the bilinear form and functional by discrete counterparts acting on $V_h$, namely $a_h: V_h \times V_h \to \mathbb{R}$ and $l_h \in V_h'$ (the construction of which is detailed further below) and

* assemble the respective basis representations of $a_h$ and $l_h$ w.r.t. the basis of $V_h$ into a matrix $\underline{a_h} \in \mathbb{R}^{N \times N}$ and vector $\underline{l_h} \in \mathbb{R}^N$, given by $(\underline{a_h})_{i, j} := a_h(\varphi_j, \varphi_i)$ and $(\underline{l_h})_i := l_h(\varphi_i)$, respectively, for $1 \leq i, j \leq N$;
* and obtain the restrictions of $\underline{a_h}$ and $\underline{l_h}$ to $V_h \cap H^1_0(\Omega)$ by modifying all entries associated with basis functions defined on the Dirichlet boundary: for each index $i \in \{1, \dots N\}$, where the Lagrange-point defining the basis function $\varphi_i$ lies on the Dirichlet boundary $\partial\Omega$, we set
  the $i$th row of $\underline{a_h}$ to a unit row and clear the $i$th entry of $\underline{l_h}$.


The algebraic version of $\eqref{discrete_variational_problem}$ then reads: find the vector of degrees of freedom (DoF) $\underline{u_h} \in \mathbb{R}^N$, such that

$$\begin{align}
\underline{a_h}\;\underline{u_h} = \underline{l_h}.\tag{7}\label{algebraic_problem}
\end{align}$$

After solving the above linear system, we recover the solution of $\eqref{discrete_variational_problem}$ from its basis representation $u_h = \sum_{i=1}^{N}\underline{u_h}_i \varphi_i$.



We consider for example a structured simplicial grid with 16 triangles.

```{code-cell}
from dune.xt.grid import Simplex, make_cube_grid, AllDirichletBoundaryInfo, visualize_grid

grid = make_cube_grid(Dim(d), Simplex(),
                      lower_left=omega[0], upper_right=omega[1], num_elements=[2, 2])
grid.global_refine(1)  # we need to refine once to obtain a symmetric grid

print(f'grid has {grid.size(0)} elements, '
      f'{grid.size(d - 1)} edges and {grid.size(d)} vertices')

boundary_info = AllDirichletBoundaryInfo(grid)

_ = visualize_grid(grid)
```

```{code-cell}
from dune.gdt import ContinuousLagrangeSpace

V_h = ContinuousLagrangeSpace(grid, order=1)

print(f'V_h has {V_h.num_DoFs} DoFs')

assert V_h.num_DoFs == grid.size(d)
```

### approximate functionals

Since the application of the functional to a *global* basis function $\psi_i$ is localizable w.r.t. the grid, e.g.

$$\begin{align}
l(\psi_i) = \sum_{K \in \mathcal{T}_h} \underbrace{\int_K f \psi_i\,\text{d}x}_{=: l^K(\psi_i)},\tag{8}\label{localized_rhs}
\end{align}$$

we first consider local functionals (such as $l^K \in L^2(K)'$), where *local* means: *with respect to a grid element $K$*. Using the reference map $F_K$ and $\eqref{basis_transformation}$ from above, we transform the evaluation of $l^K(\psi_i)$ to the reference element,

$$\begin{align}
l^K(\psi_i) &= \int_K f\psi_i\,\text{d}x = \int_{\hat{K}} |\text{det}\nabla F_K| \underbrace{(f\circ F_K)}_{=: f^K} (\hat{\psi}_\hat{i}\circ F_K^{-1}\circ F_K) \text{d}\hat{x}\\
&=\int_{\hat{K}} |\text{det}\nabla F_K| f^K \hat{\psi}_\hat{i} \,\text{d}\hat{x},\tag{9}\label{transformed_localized_rhs}
\end{align}$$

where $f^K: \hat{K} \to \mathbb{R}$ is the *local function* associated with $f$, $i = \sigma_K(\hat{i})$ and $\hat{\psi}_\hat{i}$ is the corresponding shape function.

Note that, apart from the integration domain ($\hat{K}$ instead of $K$) and the transformation factor ($|\text{det}\nabla F^K|$), the structure of the local functional from $\eqref{localized_rhs}$ is reflected in $\eqref{transformed_localized_rhs}$.

This leads us to the definition of a local functional in `dune-gdt`: ignoring the user input (namely the data function $f$ for a moment), a **local functional** is determined by

* an integrand, depending on a single test function, that we can evaluate at points on the reference element.
  We call such integrands **unary element integrand**s. In the above example, given a test basis function $\hat{\psi}$ and a point in the reference element $\hat{x}$, the integrand is determined by $\Xi^{1, K}_\text{prod}: \mathbb{P}^k(\hat{K}) \times \hat{K} \to \mathbb{R}$, $\hat{\psi}, \hat{x} \mapsto f^K(\hat{x})\,\hat{\psi}(\hat{x})$,
  which is modelled by `LocalElementProductIntegrand` in `dune-gdt` (see below); and
* an approximation of the integral in $\eqref{transformed_localized_rhs}$ by a numerical **quadrature**:
  given any unary element integrand $\Xi^{1, K}$, and $Q \in \mathbb{N}$ quadrature points $\hat{x}_1, \dots, \hat{x}_Q$ and weights $\omega_1, \dots, \omega_Q \in \mathbb{R}$, we approximate
  $l_h^K(\psi_i) := \sum_{q = 1}^Q |\text{det}\nabla F_K(\hat{x}_q)|\,\omega_q\,\Xi^{1,K}(\hat{\psi}_\hat{i}, \hat{x}_q) \approx \int_\hat{K} \Xi^{1,K}(\hat{\psi}_\hat{i}, \hat{x})\,\text{d}\hat{x} = l^K(\psi_i)$,
  which is modelled by `LocalElementIntegralFunctional` in `dune-gdt` (see below).

  Note that the order of the quadrature is determined automatically, since the integrand computes its polynomial degree given all data functions and basis functions (in the above example, the polynomial order of $f^K$ is 3 by our construction and the polynomial order of $\hat{\psi}$ is 1, since we are using piecewise linear shape functions, yielding a polynomial order of 4 for $\Xi_\text{prod}^{1,K}$).

Given local functionals, the purpose of the `VectorFunctional` in `dune-gdt` is to assemble $\underline{l_h}$ from $\eqref{algebraic_problem}$ by
* creating an appropriate vector of length $N$
* iterating over all grid elements $K \in \mathcal{T}_h$
* localizing the basis of $V_h$ w.r.t. each grid element $K$
* evaluating the local functionals $l_h^K$ for each localized basis function
* adding the results to the respective entry of $\underline{l_h}$, determined by the DoF-mapping of the discrete function space `ContinuousLagrangeSpace`


In our example, we define $l_h$ as:

```{code-cell}
from dune.xt.functions import GridFunction as GF

from dune.gdt import (
    VectorFunctional,
    LocalElementProductIntegrand,
    LocalElementIntegralFunctional,
)

l_h = VectorFunctional(grid, source_space=V_h)
l_h += LocalElementIntegralFunctional(
         LocalElementProductIntegrand(GF(grid, 1)).with_ansatz(GF(grid, f)))
```

A few notes regarding the above code:

* there exists a large variety of data functions, but in order to all handle them in `dune-gdt` we require them to be localizable w.r.t. a grid (i.e. to have *local functions* as above). This is achieved by wrapping them into a `GridFunction`, which accepts all kind of functions, discrete functions or numbers. Thus `GF(grid, 1)` creates a grid function which is localizable w.r.t. each grid elements and evaluates to 1, whenever evaluated; whereas `GF(grid, f)`, when localized to a grid element $K$ and evaluated at a point on the associated reference element, $\hat{x}$, evaluates to $f(F_K(\hat{x}))$.
  See also the [tutorial on data functions](dune_gdt_tutorial_on_data_functions_and_interpolation.md).

* the `LocalElementProductIntegrand` is actually a **binary element integrand** modelling a weighted product, as in: with a weight function $w: \Omega \to \mathbb{R}$, given an ansatz function $\hat{\varphi}$, a test function $\hat{\psi}$ and a point $\hat{x} \in \hat{K}$, this integrand is determined by
  $\Xi_\text{prod}^{2,K}: \mathbb{P}^k(\hat{K}) \times \mathbb{P}^k(\hat{K}) \times \hat{K} \to \mathbb{R}$, $\hat{\varphi}, \hat{\psi}, \hat{x} \mapsto w^K(\hat{x})\,\hat{\varphi}(\hat{x})\,\hat{\psi}(\hat{x})$.
  Thus, `LocalElementProductIntegrand` is often used in bilinear forms to assemble $L^2$ products (with weight $w =1$), which we achieve by `LocalElementProductIntegrand(GF(grid, 1))`. However, even with $w = 1$, the integrand $\Xi_\text{prod}^{2,K}$ still depends on the test and ansatz function. Using `with_ansatz(GF(grid, f))`, we fix $f^K$ as the ansatz function to obtain exactly the *unary* integrand we require, which only depends on the test function,
  $\Xi_\text{prod}^{1,K}: \mathbb{P}^k(\hat{K}) \times \hat{K} \to \mathbb{R}$, $\hat{\psi}, \hat{x} \mapsto \Xi_\text{prod}^{2, K}(f^K, \hat{\psi}, \hat{x}) = f^K(\hat{x})\,\hat{\psi}(\hat{x})$,
  which is exactly what we need to approximate  $l_\text{src}^K(\psi_i) = \int_K f\,\psi_i\text{d}x$.

* the above code creates the vector $\underline{l_h}$ (available as the `vector` attribute of `l_h`) filled with `0`, but does not yet assemble the functional into it, which we can check by:

```{code-cell}
assert len(l_h.vector) == V_h.num_DoFs

print(l_h.vector.sup_norm())
```

### approximate bilinear forms

The approximation of the application of the bilinear form $a$ to two *global* basis function $\psi_i, \varphi_j$ follows in a similar manner. We obtain by localization

$$\begin{align}
a(\psi_i, \varphi_j) &= \int_\Omega (\kappa\nabla \varphi_j)\cdot \nabla\psi_i\,\text{d}x = \sum_{K \in \mathcal{T}_h}\underbrace{\int_K (\kappa\nabla \varphi_j)\cdot \nabla\psi_i\,\text{d}x}_{=:a^K(\psi_i, \varphi_j)}
\end{align}$$

and by transformation and the chain rule, using $\eqref{basis_transformation_grad}$ and $F_K^{-1}\circ F_K = \text{id}$

$$\begin{align}
a^K(\psi_i, \varphi_j) &= \int_{\hat{K}} |\text{det}\nabla F_K| \big(\underbrace{(\kappa\circ F_K)}_{=: \kappa^K}\underbrace{(\nabla F_K^{-1}\cdot\nabla\hat{\varphi}_\hat{j})}_{=: \nabla_K\hat{\varphi}_\hat{j}}\big)\cdot\underbrace{(\nabla F_K^{-1}\cdot\nabla\hat{\psi}_\hat{i})}_{=: \nabla_K\hat{\psi}_\hat{i}}\,\text{d}\hat{x}\\
&= \int_{\hat{K}} |\text{det}\nabla F_K| \big(\kappa^K \nabla_K\hat{\varphi}_\hat{j}\big)\cdot\nabla_K\hat{\psi}_\hat{i}\,\text{d}\hat{x},
\end{align}$$

where $\kappa^K$ denote the *local function* of $\kappa$ as above, and where $\nabla_K\hat{\varphi}_\hat{j}$ denote suitably transformed *global* gradients of the *local* shape functions, for the integrand to reflect the same structure as above.

Similar to local functionals, a **local bilinear form** is determined

* by a **binary element integrand**, in our case the `LocalLaplaceIntegrand` (see below)
  $$\begin{align}
  \Xi_\text{laplace}^{2, K}: \mathbb{P}^k(\hat{K}) \times \mathbb{P}^k(\hat{K}) \times \hat{K} &\to \mathbb{R}\\
  \hat{\varphi}, \hat{\xi}, \hat{x} &\mapsto \big(\kappa^K(\hat{x})\,\nabla_K\hat{\varphi}(\hat{x})\big)\cdot\nabla_K\hat{\psi}(\hat{x})
  \end{align}$$
  and
* an approximation of the integral by a numerical **quadrature**: given any binary element integrand $\Xi^{2, K}$, and $Q \in \mathbb{N}$ quadrature points $\hat{x}_1, \dots, \hat{x}_Q$ and weights $\omega_1, \dots, \omega_Q \in \mathbb{R}$, we approximate
  $\begin{align}
  a_h^K(\psi_i, \varphi_j) := \sum_{q = 1}^Q |\text{det}\nabla F_K(\hat{x}_q)|\,\omega_q\,\Xi^{2,K}(\hat{\psi}_\hat{i}, \hat{\varphi}_\hat{j}, \hat{x}_q) \approx \int_\hat{K} \Xi^{2,K}(\hat{\psi}_\hat{i}, \hat{\varphi}_\hat{j}, \hat{x})\,\text{d}\hat{x} = a^K(\psi_i, \varphi_i),
  \end{align}$
  which is modelled by `LocalElementIntegralBilinearForm` in `dune-gdt` (see below).

Given local bilinear forms, the purpose of the `MatrixOperator` in `dune-gdt` is to assemble $\underline{a_h}$ from $\eqref{algebraic_problem}$ by
* creating an appropriate (sparse) matrix of size $N \times N$
* iterating over all grid elements $K \in \mathcal{T}_h$
* localizing the basis of $V_h$ w.r.t. each grid element $K$
* evaluating the local bilinear form $a_h^K$ for each combination localized ansatz and test basis functions
* adding the results to the respective entry of $\underline{a_h}$, determined by the DoF-mapping of the discrete function space `ContinuousLagrangeSpace`

```{code-cell}
from dune.gdt import (
    MatrixOperator,
    make_element_sparsity_pattern,
    LocalLaplaceIntegrand,
    LocalElementIntegralBilinearForm,
)

a_h = MatrixOperator(grid, source_space=V_h, range_space=V_h,
                     sparsity_pattern=make_element_sparsity_pattern(V_h))
a_h += LocalElementIntegralBilinearForm(
        LocalLaplaceIntegrand(GF(grid, kappa, dim_range=(Dim(d), Dim(d)))))
```

A few notes regarding the above code:

* the `LocalLaplaceIntegrand` expects a matrix-valued function, which we achieve by converting the scalar function `kappa` to a matrix-valued `GridFunction`

* the above code creates the matrix $\underline{a_h}$ (available as the `matrix` attribute of `a_h`) with sparse `0` entries, but does not yet assemble the bilinear form into it, which we can check by:

```{code-cell}
assert a_h.matrix.rows == a_h.matrix.cols == V_h.num_DoFs

print(a_h.matrix.sup_norm())
```

### handling the Dirichlet boundary condition

As noted above, we handle the Dirichlet boundary condition on the algebraic level by modifying the assembled matrices and vector.
We therefore require a means to identify all DoFs of $V_h$ associated with the Dirichlet boundary modelled by `boundary_info`.

```{code-cell}
from dune.gdt import DirichletConstraints

dirichlet_constraints = DirichletConstraints(boundary_info, V_h)
```

Similar to the bilinear forms and functionals above, the `dirichlet_constraints` are not yet assembled, which we can check as follows:

```{code-cell}
print(dirichlet_constraints.dirichlet_DoFs)
```

### walking the grid

Until now, we constructed a bilinear form `a_h`, a linear functional `l_h` and Dirichlet constrinaints `dirichlet_constraints`, which are all localizable w.r.t. the grid, that is: i order to compute their application or to assemble them, it is sufficient to apply them to each element of the grid.

Internally, this is realized by means of the `Walker` from `dune-xt-grid`, which allows to register all kinds of *grid functors* which are applied locally on each element. All bilinear forms, operators and functionals (as well as other constructs such as the Dirichlet constraints) in `dune-gdt` are implemented as such functors.

Thus, we may assemble everything in one grid walk:

```{code-cell}
from dune.xt.grid import Walker

walker = Walker(grid)
walker.append(a_h)
walker.append(l_h)
walker.append(dirichlet_constraints)
walker.walk()
```

We can check that the assembled bilinear form and functional as well as the Dirichlet constraints actually contain some data:

```{code-cell}
print(f'a_h = {a_h.matrix.__repr__()}')
print()
print(f'l_h = {l_h.vector.__repr__()}')
print()
print(f'Dirichlet DoFs: {dirichlet_constraints.dirichlet_DoFs}')
```

### solving the linear system

After walking the grid, the bilinra form and linear functional are assembled w.r.t. $V_h$ and we constrain them to include the handling of the Dirichlet boundary condition.

```{code-cell}
dirichlet_constraints.apply(a_h.matrix, l_h.vector)
```

Since the bilinear form is implemented as a `MatrixOperator`, we may simply invert the operator to obtain the DoF vector of the solution of $\eqref{discrete_variational_problem}$.

```{code-cell}
u_h_vector = a_h.apply_inverse(l_h.vector)
```

### postprocessing the solution

To make use of the DoF vector of the approximate solution, $\underline{u_h} \in \mathbb{R}^N$, it is convenient to interpert it as a discrete function again, $u_h \in V_h$ by means of the Galerkin isomorphism. This can be achieved by the `DiscreteFunction` in `dune-gdt`.

All discrete functions are in particular grid functions and can thus be compared to analytical solutions, used as input in discretization schemes or visualized.

**Note:** if visualization fails for some reason, call `paraview` on the command line and open `u_h.vtu`!

```{code-cell}
from dune.gdt import DiscreteFunction, visualize_function

u_h = DiscreteFunction(V_h, u_h_vector, name='u_h')
_ = visualize_function(u_h)
```

### everything in a single function

For a better overview, the above discretization code is also available in a single function in the file `discretize_elliptic_cg.py`:

```{code-cell}
import inspect
from discretize_elliptic_cg import discretize_elliptic_cg_dirichlet_zero

print(inspect.getsource(discretize_elliptic_cg_dirichlet_zero))
```

Calling it gives the same solution as above:

```{code-cell}
u_h = discretize_elliptic_cg_dirichlet_zero(grid, kappa, f)
_ = visualize_function(u_h)
```

## diffusion with non-homogeneous Dirichlet boundary condition

### analytical problem

Consider problem $\eqref{pde}$ from above, but with non-homogeneous Dirichlet boundary values. That is:


Let $\Omega \subset \mathbb{R}^d$ for $1 \leq d \leq 3$ be a bounded connected domain with Lipschitz-boundary $\partial\Omega$. We seek the solution $u \in H^1(\Omega)$ of the linear diffusion equation (with a **non-homogeneous Dirichlet boundary condition**)

$$\begin{align}
- \nabla\cdot(\kappa\nabla u) &= f &&\text{in } \Omega,\tag{10}\label{inhomogeneous_pde}\\
u &= g_\text{D} &&\text{on } \partial\Omega,
\end{align}$$

in a weak sense, where $\kappa \in [L^\infty(\Omega)]^{d \times d}$ denotes a given diffusion function, $f \in L^2(\Omega)$ denotes a given source function and $g_\text{D} \in L^2(\partial\Omega)$ denotes given Dirichlet boundary values.

The variational problem associated with $\eqref{inhomogeneous_pde}$ reads: find $u \in H^1(\Omega)$, such that

$$\begin{align}
a(u, v) &= l(v) &&\text{for all }v \in H^1_0(\Omega),\tag{11}\label{inhomogeneous_weak_formulation}
\end{align}$$

with the same bilinear form $a$ and linear functional $l$ as above in $\eqref{a_and_l}$.
<!-- #endregion -->

### Dirichlet shift

Suppose $\hat{g}_\text{D} \in H^1(\Omega)$ is an extension of the Dirichlet boundary values to $\Omega$, such that

$$\begin{align}
\hat{g}_\text{D}|_{\partial\Omega} = g_\text{D}
\end{align}$$

in the sense of traces. We then have for the solution $u \in H^1(\Omega)$ of $\eqref{inhomogeneous_weak_formulation}$, that

$$\begin{align}
u = u_0 + \hat{g}_\text{D}
\end{align}$$

for some $u_0 \in H^1_0(\Omega)$. Thus, we may reformulate $\eqref{inhomogeneous_weak_formulation}$ as follows: Find $u_0 \in H^1_0(\Omega)$, such that

$$\begin{align}
a(u_0 + \hat{g}_\text{D}, v) &= l(v) &&\text{for all }v \in H^1_0(\Omega),
\end{align}$$

or equivalently

$$\begin{align}
a(u_0, v) &= l(v) - a(\hat{g}_\text{D}, v) &&\text{for all }v \in H^1_0(\Omega).
\end{align}$$

We have thus shifted problem $\eqref{inhomogeneous_weak_formulation}$ to be of familiar form, i.e., similarly to above we consider a linear diffusion equation with **homogeneous** Dirichlet boundary conditions, but a **modified source** term.


Consider for example $\eqref{pde}$ with:

* $d = 2$
* $\Omega = [0, 1]^2$
* $\kappa = 1$
* $f = \exp(x_0 x_1)$
* $g_\text{D} = x_0 x_1$

Except for $g_\text{D}$, we may thus use the same grid, boundary info and data functions as above.


### Dirichlet interpolation

To obtain $\hat{g}_\text{D} \in H^1(\Omega)$, we use the Lagrange interpolation of $g_\text{D}$ and set all DoFs associated with inner Lagrange nodes to zero. This is realized by the `boundary_interpolation`.

```{code-cell}
from dune.xt.grid import DirichletBoundary
from dune.gdt import boundary_interpolation

g_D = ExpressionFunction(dim_domain=Dim(d),
                         variable='x', expression='x[0]*x[1]', order=2, name='g_D')

g_D_hat = boundary_interpolation(GF(grid, g_D), V_h, boundary_info, DirichletBoundary())

_ = visualize_function(g_D_hat)
```

As we observe, the values on all boundary DoFs are $x_0 x_1$ and on all inner DoFs $0$.


### assembling the shifted system

* We assemble the unconstrained matrix- and vector representation of $a_h$ and $l_h$ w.r.t. $V_h$ similarly as above.

```{code-cell}
l_h = VectorFunctional(grid, source_space=V_h)
l_h += LocalElementIntegralFunctional(
        LocalElementProductIntegrand(GF(grid, 1)).with_ansatz(GF(grid, f)))

a_h = MatrixOperator(grid, source_space=V_h, range_space=V_h,
                     sparsity_pattern=make_element_sparsity_pattern(V_h))
a_h += LocalElementIntegralBilinearForm(
        LocalLaplaceIntegrand(GF(grid, kappa, dim_range=(Dim(d), Dim(d)))))

walker = Walker(grid)
walker.append(a_h)
walker.append(l_h)
walker.walk()
```

* We then obtain the shifted system by directly modifying the right hand side vector.

```{code-cell}
rhs_vector = l_h.vector - a_h.matrix@g_D_hat.dofs.vector
```

* *Afterwards*, we constrain the shifted system to $V_h \cap H^1_0(\Omega)$.

```{code-cell}
dirichlet_constraints.apply(a_h.matrix, rhs_vector)
```

* Then, we solve the shifted and constrained system for the DoF vector of $u_{0, h}$.

```{code-cell}
u_0_h_vector = a_h.apply_inverse(rhs_vector)
```

* We may then obtain the solution by shifting it back.

```{code-cell}
u_h = DiscreteFunction(V_h, u_0_h_vector + g_D_hat.dofs.vector)

_ = visualize_function(u_h)
```

### everything in a single function

As above, solving with non-homogeneous Dirichlet values is also available in a single function.

```{code-cell}
from discretize_elliptic_cg import discretize_elliptic_cg_dirichlet

u_h = discretize_elliptic_cg_dirichlet(grid, kappa, f, g_D)

_ = visualize_function(u_h)
```

Download the code:
{download}`dune_gdt_tutorial_on_cg_fem_for_the_stationary_heat_equation.md`
{nb-download}`dune_gdt_tutorial_on_cg_fem_for_the_stationary_heat_equation.ipynb`
