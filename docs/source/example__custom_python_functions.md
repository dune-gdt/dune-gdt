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

# Example: custom Python functions as C++ (grid) functions

So far, every coefficient function in these examples has been either a constant
(`ConstantFunction`) or a parsed expression string (`ExpressionFunction`, e.g.
`'exp(x[0]*x[1])'`). The expression parser is convenient, but it is also a fragile input channel:
it understands only what its grammar implements, and it silently mishandles some inputs (e.g. it
rejects subnormal float literals).

`dune.xt.functions.GenericFunction` and `GenericGridFunction` close that gap: instead of a string
to be parsed, they take a plain **Python callable** and invoke it directly from the C++ grid walk.
This means any Python code -- piecewise definitions, random coefficients, calls into numpy/scipy --
can serve as a `dune-gdt` coefficient function.

* `GenericFunction` is evaluated in *global* (physical) coordinates, like `ExpressionFunction`.
* `GenericGridFunction` is evaluated *locally*, in reference-element coordinates, and additionally
  supports a `post_bind` callback invoked once per element during the grid walk -- so the function
  can depend on the element itself (its index, its center, ...), not just on the point being
  evaluated.

Both take an `order` (the polynomial degree used to pick a quadrature rule) exactly like
`ExpressionFunction`, since the callback is opaque to the C++ side; there is no way to infer the
polynomial degree of a Python function automatically.

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer

import time

import numpy as np

from dune.xt.grid import Dim

try:
    from dune.xt.functions import GenericFunction, GenericGridFunction

    HAVE_GENERIC_FUNCTION = True
except ImportError:
    HAVE_GENERIC_FUNCTION = False
    print(
        "This wheel was not built with the GenericFunction/GenericGridFunction bindings, so the\n"
        "cells below are skipped instead of failing."
    )
```

## 1: `GenericFunction` vs. `ExpressionFunction`

We solve the same stationary heat equation as in
{download}`example__ipdg_stationary_heat_equation.md`, once with the diffusion and source given as
`ExpressionFunction` (a parsed string), and once with the *same* math given as a `GenericFunction`
(a Python lambda). The two must produce the same discrete solution, since they describe the same
function through two independent code paths.

```{code-cell}
d = 2
omega = ([0, 0], [1, 1])

if HAVE_GENERIC_FUNCTION:
    from dune.xt.functions import ConstantFunction, ExpressionFunction

    kappa_expression = ConstantFunction(
        dim_domain=Dim(d), dim_range=Dim(1), value=[1.0], name="kappa"
    )
    f_expression = ExpressionFunction(
        dim_domain=Dim(d),
        variable="x",
        expression="exp(x[0]*x[1])",
        order=3,
        name="f",
    )

    # the same source term, but evaluated by a Python callable instead of the expression parser
    def f_evaluate(x, mu):
        return [float(np.exp(x[0] * x[1]))]

    f_generic = GenericFunction(
        dim_domain=Dim(d), dim_range=Dim(1), order=3, evaluate=f_evaluate, name="f_generic"
    )
```

```{code-cell}
from dune.xt.grid import Simplex, make_cube_grid

if HAVE_GENERIC_FUNCTION:
    grid = make_cube_grid(Dim(d), Simplex(), lower_left=omega[0], upper_right=omega[1], num_elements=[2, 2])
    grid.global_refine(1)  # we need to refine once to obtain a symmetric grid
    print(f"grid has {grid.size(0)} elements, {grid.size(d - 1)} edges and {grid.size(d)} vertices")
```

```{code-cell}
if HAVE_GENERIC_FUNCTION:
    from discretize_elliptic_ipdg import discretize_elliptic_ipdg_dirichlet_zero

    u_h_expression = discretize_elliptic_ipdg_dirichlet_zero(grid, kappa_expression, f_expression)
    u_h_generic = discretize_elliptic_ipdg_dirichlet_zero(grid, kappa_expression, f_generic)

    dofs_expression = np.array(u_h_expression.dofs.vector, copy=False)
    dofs_generic = np.array(u_h_generic.dofs.vector, copy=False)
    max_difference = np.abs(dofs_expression - dofs_generic).max()
    print(f"max |dofs_expression - dofs_generic| = {max_difference:.3e}")
    assert max_difference < 1e-13
```

```{code-cell}
if HAVE_GENERIC_FUNCTION:
    from dune.gdt import visualize_function

    _ = visualize_function(u_h_generic)
```

## 2: `GenericGridFunction` with `post_bind` -- an element-dependent coefficient

A `GenericFunction` can only see the (global) evaluation point, so it cannot depend on *which*
element it is being evaluated on -- e.g. to implement a piecewise-constant "checkerboard"
coefficient without going through `dune.xt.functions.CheckerboardFunction`. `GenericGridFunction`'s
`post_bind` callback is invoked once whenever the grid walk moves to a new element, before any
`evaluate` calls on that element, so we can look up a per-element value there and stash it in a
Python closure for `evaluate` to read:

```{code-cell}
if HAVE_GENERIC_FUNCTION:
    def make_checkerboard_diffusion(grid, blocks, values):
        """A piecewise-constant, element-wise isotropic diffusion tensor kappa(x) = values[cell(x)].

        `values` is indexed by the coarse "checkerboard" cell (row-major over `blocks`) the
        element's center falls into.
        """
        d = grid.dimension
        blocks = np.asarray(blocks[:d])
        state = {"value": values[0]}

        def post_bind(element):
            cell = np.minimum((np.asarray(element.center) * blocks).astype(int), blocks - 1)
            state["value"] = values[int(np.ravel_multi_index(tuple(cell), tuple(blocks)))]

        def evaluate(x, mu):
            value = float(state["value"])
            return [[value if i == j else 0.0 for j in range(d)] for i in range(d)]

        # dim_range disambiguates the scalar/vector/matrix GenericGridFunction overloads sharing
        # this factory name -- it is not otherwise inferable from the post_bind/evaluate callables
        return GenericGridFunction(
            grid,
            order=0,
            post_bind=post_bind,
            evaluate=evaluate,
            dim_range=(Dim(d), Dim(d)),
            name="checkerboard_kappa",
        )

    rng = np.random.default_rng(0)
    checkerboard_values = rng.uniform(0.1, 10.0, size=4)
    kappa_checkerboard = make_checkerboard_diffusion(grid, blocks=(2, 2), values=checkerboard_values)

    u_h_checkerboard = discretize_elliptic_ipdg_dirichlet_zero(grid, kappa_checkerboard, f_expression)
    _ = visualize_function(u_h_checkerboard)
```

## 3: overhead of a Python callback vs. a parsed expression

Every `GenericFunction`/`GenericGridFunction` evaluation calls back into Python, which means
pybind11 has to acquire the GIL for each call (this happens automatically as part of the
`std::function` <-> Python-callable conversion pybind11 generates -- no code in the bindings
manages the GIL explicitly). `ExpressionFunction` never leaves C++. We measure the per-call
overhead by assembling the same Laplace matrix with each and dividing the wall-clock difference by
the number of `evaluate` calls (elements times quadrature points times repetitions):

```{code-cell}
if HAVE_GENERIC_FUNCTION:
    from dune.gdt import (
        BilinearForm,
        ContinuousLagrangeSpace,
        LocalElementIntegralBilinearForm,
        LocalLaplaceIntegrand,
        MatrixOperator,
        make_element_sparsity_pattern,
    )
    from dune.xt.functions import GridFunction as GF

    def assemble_laplace(grid, diffusion, repetitions=5):
        space = ContinuousLagrangeSpace(grid, order=1)
        start = time.perf_counter()
        for _ in range(repetitions):
            form = BilinearForm(grid)
            form += LocalElementIntegralBilinearForm(
                LocalLaplaceIntegrand(GF(grid, diffusion, dim_range=(Dim(d), Dim(d))))
            )
            op = MatrixOperator(
                grid,
                source_space=space,
                range_space=space,
                sparsity_pattern=make_element_sparsity_pattern(space),
            )
            op.append(form)
            op.assemble()
        return time.perf_counter() - start

    fine_grid = make_cube_grid(Dim(d), Simplex(), lower_left=omega[0], upper_right=omega[1], num_elements=[8, 8])
    fine_grid.global_refine(1)
    num_elements = fine_grid.size(0)
    # order-1 CG on a 2d simplex: a 2nd order quadrature rule has 3 points per element
    quadrature_points_per_element = 3
    repetitions = 5

    time_expression = assemble_laplace(fine_grid, kappa_expression, repetitions)

    # scalar, matching kappa_expression exactly (GF's dim_range=(Dim(d), Dim(d)) tag in
    # assemble_laplace promotes both identically) -- so the timing difference below isolates the
    # cost of the Python callback itself, not an unrelated scalar-vs-matrix evaluation difference
    def kappa_evaluate(x, mu):
        return [1.0]

    kappa_generic = GenericFunction(
        dim_domain=Dim(d), dim_range=Dim(1), order=0, evaluate=kappa_evaluate, name="kappa_generic"
    )
    time_generic = assemble_laplace(fine_grid, kappa_generic, repetitions)

    total_calls = num_elements * quadrature_points_per_element * repetitions
    overhead_per_call = (time_generic - time_expression) / total_calls
    print(f"ExpressionFunction assembly: {time_expression:.4f} s")
    print(f"GenericFunction assembly:    {time_generic:.4f} s")
    print(f"~{total_calls} evaluate() calls total")
    print(f"estimated overhead per Python callback: {overhead_per_call * 1e6:.2f} us")
```

This overhead (typically low single-digit microseconds per call on CPython, dominated by the GIL
acquisition and the argument/return-value conversion) is the price of the flexibility gained: any
Python code, however irregular, becomes usable as a coefficient function.

Download the code:
{download}`example__custom_python_functions.md`
{nb-download}`example__custom_python_functions.ipynb`
