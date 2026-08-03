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
Sections 4--8 then exercise the follow-up bindings on the same problem: automatic time step
estimation (`estimate_dt_for_hyperbolic_system`), adaptive Runge-Kutta time stepping
(`DormandPrinceTimeStepper`), a discontinuous Galerkin discretization (`AdvectionDgOperator`),
slope-limited linear reconstruction of cell averages (`LinearReconstructionOperator`) and
dimensional operator splitting (`StrangSplittingTimeStepper`).

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
    # ExpressionFunction's r == 1 (scalar range) overload takes a single "expression" string, not
    # the "expressions" list the r > 1 overloads take -- the bump is always scalar-valued (r == 1).
    return ExpressionFunction(
        dim_domain=Dim(dim),
        variable="x",
        expression=expression,
        order=2,
        name="gaussian_bump",
    )


def gaussian_bump(x, center, sigma, amplitude=1.0):
    r2 = sum((xi - ci) ** 2 for xi, ci in zip(x, center))
    return amplitude * math.exp(-r2 / (2.0 * sigma**2))


def linear_transport_flux_expression(velocity):
    # f(u) = velocity * u, as a function of the *state* u (dim_domain=1), not of x. Same
    # "expression" (singular, r == 1) vs. "expressions" (list, r > 1) split as gaussian_bump_expression.
    #
    # NumericalUpwindFlux evaluates the flux's Jacobian internally to pick the upwind side, so
    # gradient_expressions must be supplied: d(flux_i)/du = velocity[i] (a constant, since f is
    # linear in u).
    expressions = [f"({c!r})*u[0]" for c in velocity]
    gradients = [f"{c!r}" for c in velocity]
    if len(expressions) == 1:
        return ExpressionFunction(
            dim_domain=Dim(1),
            variable="u",
            expression=expressions[0],
            gradient_expressions=[gradients[0]],
            order=1,
            name="linear_transport_flux",
        )
    return ExpressionFunction(
        dim_domain=Dim(1),
        variable="u",
        expressions=expressions,
        gradient_expressions=[[g] for g in gradients],
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

numerical_flux = NumericalUpwindFlux(grid, linear_transport_flux_expression(velocity))
op = AdvectionFvOperator(space, numerical_flux)
op.boundary_treatment(lambda u: u)  # zero-order extrapolation; never actually reached here

h = (domain[1][0] - domain[0][0]) / 64
dt = 0.4 * h / abs(velocity[0])  # CFL number 0.4

time_stepper = ExplicitEulerTimeStepper(op, u_0, r=-1.0)
```

```{admonition} Sign convention
:class: note
`ExplicitEulerTimeStepper` (and every `ExplicitRungeKutta*TimeStepper`) solves $u_t = r \cdot L(u)$ for
the operator $L$ passed to it. `AdvectionFvOperator.apply(u)` computes $L(u) = +\operatorname{div}(f(u))$
(the numerical flux leaving each cell, divided by its volume), so recovering the conservation law
$u_t + \operatorname{div}(f(u)) = 0$ requires $r = -1$.
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
    op_i = AdvectionFvOperator(
        space_i, NumericalUpwindFlux(grid_i, linear_transport_flux_expression(velocity))
    )
    op_i.boundary_treatment(lambda u: u)

    h_i = (domain[1][0] - domain[0][0]) / num_elements
    dt_i = 0.4 * h_i / abs(velocity[0])
    stepper_i = ExplicitEulerTimeStepper(op_i, u_0_i, r=-1.0)
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
    space_2d, NumericalUpwindFlux(grid_2d, linear_transport_flux_expression(velocity_2d))
)
op_2d.boundary_treatment(lambda u: u)

h_2d = 10.0 / 48
speed_2d = math.sqrt(sum(c**2 for c in velocity_2d))
dt_2d = 0.4 * h_2d / speed_2d
stepper_2d = ExplicitEulerTimeStepper(op_2d, u_0_2d, r=-1.0)
for _ in range(80):
    stepper_2d.step(dt_2d)

_ = visualize_function(stepper_2d.current_solution())
```

## 4: choosing `dt` automatically

Instead of hand-rolling the CFL formula, `estimate_dt_for_hyperbolic_system` estimates a stable
time step length following [Cockburn, Coquel, LeFloch, 1995] from the state's data range, the
flux derivative on that range and the grid's worst perimeter/volume ratio. For linear transport on
an equidistant 1d grid the estimate is exactly $h / (2 |v|)$, i.e. CFL number $0.5$:

```{code-cell}
from dune.gdt import estimate_dt_for_hyperbolic_system

u_state = default_interpolation(
    GridFunction(grid, gaussian_bump_expression(1, center, sigma)), space
)
dt_estimated = estimate_dt_for_hyperbolic_system(
    grid, u_state, linear_transport_flux_expression(velocity)
)
print(f"estimated dt = {dt_estimated:.5f}")
print(f"h / (2 |v|)  = {(10.0 / 64) / (2 * abs(velocity[0])):.5f}")
print(f"hand-picked  = {dt:.5f}  (CFL number 0.4)")
```

## 5: adaptive time stepping

`DormandPrinceTimeStepper` (embedded RK45; `BogackiShampineTimeStepper` is the cheaper embedded
RK23) repeats each step until an internal error estimate meets `tol` and returns a suggested length
for the next step, so no CFL bookkeeping is needed at all -- `dt` only seeds the first attempt.
`AdaptiveRungeKuttaTimeStepper` is an alias for the Dormand-Prince variant, matching the C++
default:

```{code-cell}
from dune.gdt import DormandPrinceTimeStepper

u_adaptive = default_interpolation(
    GridFunction(grid, gaussian_bump_expression(1, center, sigma)), space
)
op_adaptive = AdvectionFvOperator(
    space, NumericalUpwindFlux(grid, linear_transport_flux_expression(velocity))
)
op_adaptive.boundary_treatment(lambda u: u)

adaptive_stepper = DormandPrinceTimeStepper(op_adaptive, u_adaptive, r=-1.0, tol=1e-4)

T_adaptive = 2.0
suggested_dt = dt_estimated
num_steps = 0
while adaptive_stepper.current_time() < T_adaptive - 1e-10:
    max_dt = T_adaptive - adaptive_stepper.current_time()
    suggested_dt = adaptive_stepper.step(suggested_dt, max_dt)
    num_steps += 1

_ = visualize_function(adaptive_stepper.current_solution(), grid)
plt.title(f"adaptive RK45: t = {adaptive_stepper.current_time():.2f} in {num_steps} steps")
```

## 6: higher order in space: discontinuous Galerkin

`AdvectionDgOperator` discretizes the same conservation law with a discontinuous Galerkin scheme on
a `DiscontinuousLagrangeSpace` (order 1 here), reusing the very same numerical flux at the element
interfaces. Two differences to the FV operator: it has to be `assemble()`d once before use (it
precomputes the local mass matrices it inverts in every application), and the explicit CFL
condition is stricter by roughly $1 / (2\,\text{order} + 1)$:

```{code-cell}
from dune.gdt import (
    AdvectionDgOperator,
    DiscontinuousLagrangeSpace,
    ExplicitRungeKutta2TimeStepper,
)

dg_space = DiscontinuousLagrangeSpace(grid, order=1)
u_dg = default_interpolation(
    GridFunction(grid, gaussian_bump_expression(1, center, sigma)), dg_space
)
op_dg = AdvectionDgOperator(
    dg_space, NumericalUpwindFlux(grid, linear_transport_flux_expression(velocity))
)
op_dg.boundary_treatment(lambda u: u)
op_dg.assemble()

dt_dg = dt_estimated / 3.0  # 1 / (2 order + 1) for order 1
stepper_dg = ExplicitRungeKutta2TimeStepper(op_dg, u_dg, r=-1.0)
T_dg = 2.0
while stepper_dg.current_time() < T_dg - 1e-10:
    stepper_dg.step(min(dt_dg, T_dg - stepper_dg.current_time()))

_ = visualize_function(stepper_dg.current_solution(), grid)
plt.title(f"DG order 1 + SSP-RK2: t = {stepper_dg.current_time():.2f}")
```

Compared with the first-order FV solution after the same time (section 1's snapshots), the bump's
peak is preserved noticeably better -- the expected qualitative gain of the (formally second-order)
DG(1) scheme over FV on a smooth solution.

## 7: piecewise linear reconstruction of cell averages

`LinearReconstructionOperator` turns the piecewise constant FV cell averages into a slope-limited
piecewise linear (order-1 discontinuous Lagrange) function -- the spatial reconstruction step of
MUSCL-type second-order FV schemes. The `slope` limiter is chosen by name (`"minmod"` (default),
`"mc"`, `"superbee"`, `"central"` or `"no_reconstruction"`); the boundary values fill the
reconstruction stencils of the boundary cells (our bump is numerically zero there, so passing the
initial-data expression is fine). Limiting tilts each cell average without moving it, so the cell
averages -- and with them the total mass -- are preserved exactly:

```{code-cell}
from dune.gdt import DiscreteFunction, LinearReconstructionOperator

u_fv = default_interpolation(
    GridFunction(grid, gaussian_bump_expression(1, center, sigma)), space
)
reconstruction_op = LinearReconstructionOperator(
    space,
    linear_transport_flux_expression(velocity),
    gaussian_bump_expression(1, center, sigma),
    slope="minmod",
)
u_reconstructed = DiscreteFunction(
    dg_space, reconstruction_op.apply(u_fv.dofs.vector), name="reconstruction"
)

h = 10.0 / 64
mass_fv = float(np.sum(np.asarray(u_fv.dofs.vector, dtype=float)) * h)
mass_rec = float(np.sum(np.asarray(u_reconstructed.dofs.vector, dtype=float)) * h / 2)
print(f"mass of the cell averages:  {mass_fv:.12f}")
print(f"mass of the reconstruction: {mass_rec:.12f}")

_ = visualize_function(u_reconstructed, grid)
plt.title("minmod-limited linear reconstruction of the FV cell averages")
```

## 8: operator splitting in 2d

The fractional-step time steppers compose two (arbitrary, already-constructed) steppers: in each
step of `FractionalStepTimeStepper` the first stepper advances the common state to $t + \Delta t$,
then the second (Godunov splitting); `StrangSplittingTimeStepper` symmetrizes this (first to
$t + \Delta t/2$, second to $t + \Delta t$, first again to $t + \Delta t$) to restore second-order
accuracy in time. Here the 2d transport is split dimensionally, $v = (v_0, 0) + (0, v_1)$, with
both substeppers evolving the *same* `DiscreteFunction`:

```{code-cell}
from dune.gdt import StrangSplittingTimeStepper

u_split = default_interpolation(
    GridFunction(grid_2d, gaussian_bump_expression(2, center_2d, sigma_2d)), space_2d
)
substeppers = []
for directional_velocity in [(velocity_2d[0], 0.0), (0.0, velocity_2d[1])]:
    op_directional = AdvectionFvOperator(
        space_2d,
        NumericalUpwindFlux(
            grid_2d, linear_transport_flux_expression(directional_velocity)
        ),
    )
    op_directional.boundary_treatment(lambda u: u)
    substeppers.append(ExplicitEulerTimeStepper(op_directional, u_split, r=-1.0))

splitting_stepper = StrangSplittingTimeStepper(substeppers[0], substeppers[1])

volumes_2d = []
grid_2d.apply_on_each_element(lambda e: volumes_2d.append(e.volume))
mass = lambda u: float(  # noqa: E731
    np.dot(np.asarray(u.dofs.vector, dtype=float), np.asarray(volumes_2d))
)

mass_before = mass(u_split)
for _ in range(80):
    splitting_stepper.step(dt_2d)
print(f"mass before: {mass_before:.12f}")
print(f"mass after:  {mass(splitting_stepper.current_solution()):.12f}")

_ = visualize_function(splitting_stepper.current_solution())
```

The bump ends up in the same place as in section 3's unsplit evolution, and the discrete mass is
conserved -- each substep is a mass-conserving FV update.

Download the code:
{download}`example__linear_transport_fv.md`
{nb-download}`example__linear_transport_fv.ipynb`
