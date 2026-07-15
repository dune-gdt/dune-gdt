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

# Example: linear transport via finite volumes

This example solves the linear transport equation

$$
\partial_t u + \operatorname{div}(v\, u) = 0 \quad \text{in } \Omega \times (0, T],
$$

for a constant velocity $v$, using a first-order finite volume (FV) upwind scheme in space and
explicit Euler in time -- the same discretization as the C++ `linear-transport/` test suite
(`dune/gdt/test/linear-transport/`), but assembled here from the Python bindings added for #320
WP6: `dune.gdt.NumericalUpwindFlux`, `AdvectionFvOperator` and `ExplicitEulerTimeStepper`.

```{admonition} No periodic grid views yet
:class: note
The C++ tests run this problem on a periodic domain. `dune.xt.grid` does not (yet) bind a periodic
grid view from Python, so this example uses a plain (non-periodic) box instead: the initial data is
a Gaussian bump centered in the domain that decays to floating point noise well before the boundary,
so the (arbitrary) boundary treatment never actually influences the solution during the simulation.
```

```{code-cell}
# wurlitzer: display dune's output in the notebook
%load_ext wurlitzer

import math

import numpy as np
from matplotlib import pyplot as plt

from dune.gdt import (
    AdvectionFvOperator,
    ExplicitEulerTimeStepper,
    FiniteVolumeSpace,
    NumericalUpwindFlux,
    default_interpolation,
    visualize_function,
)
from dune.xt.functions import ExpressionFunction, GridFunction
from dune.xt.grid import Cube, Dim, make_cube_grid
```

## 1: a moving Gaussian bump in 1d

A helper builds both the (`ExpressionFunction`) initial data and the exact translated solution
$u(x, t) = u_0(x - v t)$ from the same closed-form Gaussian, so the two never drift apart:

```{code-cell}
def gaussian_bump_expression(dim, center, sigma, amplitude=1.0):
    terms = "+".join(
        f"(x[{i}]-({center[i]!r}))*(x[{i}]-({center[i]!r}))" for i in range(dim)
    )
    expression = f"({amplitude!r})*exp(-({terms})/(2.0*({sigma!r})*({sigma!r})))"
    return ExpressionFunction(
        dim_domain=Dim(dim),
        variable="x",
        expressions=[expression],
        order=2,
        name="gaussian_bump",
    )


def gaussian_bump(x, center, sigma, amplitude=1.0):
    r2 = sum((xi - ci) ** 2 for xi, ci in zip(x, center))
    return amplitude * math.exp(-r2 / (2.0 * sigma**2))


def linear_transport_flux_expression(velocity):
    # f(u) = velocity * u, as a function of the *state* u (dim_domain=1), not of x
    expressions = [f"({c!r})*u[0]" for c in velocity]
    return ExpressionFunction(
        dim_domain=Dim(1),
        variable="u",
        expressions=expressions,
        order=1,
        name="linear_transport_flux",
    )
```

Domain $[0, 10]$, velocity $v = 1$, bump centered at $x = 2$ with $\sigma = 0.3$ (comfortably many
$\sigma$ away from either boundary for the duration of the simulation):

```{code-cell}
domain = ([0.0], [10.0])
velocity = (1.0,)
center = (2.0,)
sigma = 0.3

grid = make_cube_grid(Dim(1), lower_left=domain[0], upper_right=domain[1], num_elements=[64])
space = FiniteVolumeSpace(grid)

u_0 = default_interpolation(
    GridFunction(grid, gaussian_bump_expression(1, center, sigma)), space
)

numerical_flux = NumericalUpwindFlux(linear_transport_flux_expression(velocity))
op = AdvectionFvOperator(space, numerical_flux)
op.boundary_treatment(lambda u: u)  # zero-order extrapolation; never actually reached here

h = (domain[1][0] - domain[0][0]) / 64
dt = 0.4 * h / abs(velocity[0])  # CFL number 0.4

time_stepper = ExplicitEulerTimeStepper(op, u_0)
```

A handful of snapshots, taken every 20 explicit Euler steps, show the bump advecting to the right:

```{code-cell}
num_steps_per_frame = 20
for frame in range(5):
    _ = visualize_function(time_stepper.current_solution(), grid)
    plt.title(f"t = {time_stepper.current_time():.2f}")
    for _ in range(num_steps_per_frame):
        time_stepper.step(dt)
```

## 2: experimental order of convergence (EOC)

Since the flux is linear, the exact solution is the initial bump translated by $v T$; refining the
mesh (halving `h`, hence `dt`, at fixed CFL number) should reduce the $L^2$ error at a fixed final
time $T$ at the first-order rate the upwind scheme is expected to have -- the same quantity the C++
`instationary-eocstudies` machinery reports for this problem:

```{code-cell}
def l2_error_at(grid, discrete_function, exact_at_t):
    centers, volumes = [], []
    grid.apply_on_each_element(
        lambda e: (centers.append(np.array(e.center, copy=False).copy()), volumes.append(e.volume))
    )
    u_h = np.asarray(discrete_function.dofs.vector, dtype=float)
    u_exact = np.array([exact_at_t(x) for x in centers])
    return float(np.sqrt(np.sum((u_h - u_exact) ** 2 * np.asarray(volumes))))


T = 1.0
errors = []
resolutions = [32, 64, 128]
for num_elements in resolutions:
    grid_i = make_cube_grid(
        Dim(1), lower_left=domain[0], upper_right=domain[1], num_elements=[num_elements]
    )
    space_i = FiniteVolumeSpace(grid_i)
    u_0_i = default_interpolation(
        GridFunction(grid_i, gaussian_bump_expression(1, center, sigma)), space_i
    )
    op_i = AdvectionFvOperator(space_i, NumericalUpwindFlux(linear_transport_flux_expression(velocity)))
    op_i.boundary_treatment(lambda u: u)

    h_i = (domain[1][0] - domain[0][0]) / num_elements
    dt_i = 0.4 * h_i / abs(velocity[0])
    stepper_i = ExplicitEulerTimeStepper(op_i, u_0_i)
    t = 0.0
    while t < T - 1e-10:
        actual_dt = min(dt_i, T - t)
        stepper_i.step(actual_dt, actual_dt)
        t = stepper_i.current_time()

    exact_at_T = lambda x, t=T: gaussian_bump(  # noqa: E731
        tuple(xi - v * t for xi, v in zip(x, velocity)), center, sigma
    )
    errors.append(l2_error_at(grid_i, stepper_i.current_solution(), exact_at_T))

print(f"{'elements':>10}  {'L2 error':>12}  {'EOC':>6}")
for i, (num_elements, error) in enumerate(zip(resolutions, errors)):
    eoc = "--" if i == 0 else f"{math.log2(errors[i - 1] / error):.2f}"
    print(f"{num_elements:>10}  {error:>12.6f}  {eoc:>6}")
```

The observed order should be close to 1 -- the expected rate for a first-order FV upwind scheme,
matching what `dune/gdt/test/instationary-eocstudies/hyperbolic-nonconforming.hh` reports for the
same discretization in C++.

## 3: the same setup in 2d

`AdvectionFvOperator`/`NumericalUpwindFlux` are bound for every already-bound grid dimension, so the
2d case only differs in the domain, grid and velocity:

```{code-cell}
grid_2d = make_cube_grid(
    Dim(2), Cube(), lower_left=[0.0, 0.0], upper_right=[10.0, 10.0], num_elements=[48, 48]
)
space_2d = FiniteVolumeSpace(grid_2d)

velocity_2d = (1.0, 0.5)
center_2d = (2.5, 2.5)
sigma_2d = 0.4

u_0_2d = default_interpolation(
    GridFunction(grid_2d, gaussian_bump_expression(2, center_2d, sigma_2d)), space_2d
)
op_2d = AdvectionFvOperator(
    space_2d, NumericalUpwindFlux(linear_transport_flux_expression(velocity_2d))
)
op_2d.boundary_treatment(lambda u: u)

h_2d = 10.0 / 48
speed_2d = math.sqrt(sum(c**2 for c in velocity_2d))
dt_2d = 0.4 * h_2d / speed_2d
stepper_2d = ExplicitEulerTimeStepper(op_2d, u_0_2d)
for _ in range(80):
    stepper_2d.step(dt_2d)

_ = visualize_function(stepper_2d.current_solution())
```

Download the code:
{download}`example__linear_transport_fv.md`
{nb-download}`example__linear_transport_fv.ipynb`
