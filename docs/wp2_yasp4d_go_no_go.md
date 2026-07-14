# WP2 (#320): `YASP_4D` in the Python bindings — go / no-go

**Decision: NO-GO for now — do not add `YASP_4D_EQUIDISTANT_OFFSET` to the bindings in WP2.**

This is the written 4d evaluation that WP2 of
[issue #320](https://github.com/dune-gdt/dune-gdt/issues/320) asks for
("`YASP_4D` is *evaluation only*: measure the template cost of one 4d grid across the space/operator
binding TUs before committing; visualization and gmsh are N/A in 4d"). WP2 *ships* the 1d
structured grid (`YASP_1D`) and *evaluates* the 4d one; only the former lands as bindings.

## What "adding `YASP_4D`" would cost

`YASP_4D_EQUIDISTANT_OFFSET` already exists as a C++ type alias
(`dune/xt/grid/grids.hh`) and the C++ `CubicGrids` typed tests already exercise it via
`dune/dgf_4d_interval.dgf`. The cost is therefore *not* new C++ — it is the pybind11 instantiation
blow-up if the type is added to `Dune::XT::Grid::bindings::AvailableGridTypes`.

Every binding translation unit that walks `bindings::AvailableGridTypes` (directly or through a
`*_for_all_grids` helper) gains **one more full grid instantiation** — space mappers, local finite
elements, operator/functional assembly over the 4d leaf view, discrete-function DoF handling, the
whole stack — per added grid type. Counted statically on this branch:

| Instantiation-driving binding TUs | count |
|---|---|
| `python/gdt/**/*.cc` over `AvailableGridTypes` / `*_for_all_grids` | 68 |
| `python/xt/**/*.cc` over `AvailableGridTypes` / `*_for_all_grids`  | 35 |
| **total** | **103** |
| of which `python/gdt/dune/gdt/spaces/*.cc`    | 7 |
| of which `python/gdt/dune/gdt/operators/*.cc` | 9 |

So a single 4d grid adds an instantiation to ~103 of the 129 binding TUs.

### Why 4d is disproportionately expensive (not just "one more grid")

The pybind11 binding TUs are already the **CI memory driver**. Per `non_docker_build.yml`
(the `config` sizing job and the build-linux comments), the ordinary library/test TUs peak ~2 GB,
but the binding TUs "peak far higher (well above 4 GB each)"; building them at the shared `cpu-1`
level **OOM-killed the 16 GB GitHub-hosted runner**, which is why they run under a tighter
`bindings_jobs = cpu/2` cap. `operators/interfaces_istl_{1d,2d,3d}.cc` was already split by
dimension for exactly this reason.

A 4d `YaspGrid` instantiation is deeper than the existing 2d/3d ones, not equal to them:

- **Codimension count scales with dimension.** A 4d grid has entities of codim 0..4 (5 levels vs. 4
  in 3d, 3 in 2d); mappers, sparsity patterns, intersection handling and reference-element geometry
  instantiate per codim.
- **Local finite elements / quadratures** are templated on dimension and geometry type; the 4d
  reference cube's shape-function and quadrature templates are new, heavier instantiations.
- **`FieldMatrix`/`FieldVector` geometry** (`4`- and `16`-entry jacobians/inverses) instantiates
  fresh throughout the assembly code.

The concrete peak-RAM and wall-clock delta must be **measured in the CI toolchain** (see recipe
below) — it cannot be produced from the source checkout alone, because it depends on the compiler,
the Dune headers as configured by `config.h`, and pybind11. The cost guardrail in #320 requires that
number *and* a further TU split for any TU that then exceeds ~4 GB. Given the binding TUs already
sit "well above 4 GB", the realistic expectation is that adding 4d pushes several `spaces/*` and
`operators/*` TUs over the OOM threshold and forces another round of by-dimension TU splitting on
top of the raw compile-time increase across all 103 TUs.

## What 4d would buy

Little, relative to the cost:

- **No visualization** — VTK/`visualize_grid`/`visualize_function` are undefined in 4d, so a 4d
  example notebook (the usual WP deliverable and CI smoke test) cannot show anything visual.
- **No gmsh** — 4d meshing is N/A, so no unstructured 4d counterpart is reachable anyway.
- **Property-test value is marginal.** The runtime property suite (#319) already covers the
  dimension-generic invariants (element/vertex counts, refinement factors, intersection partition,
  DoF-count formulas) at 1d/2d/3d; 4d adds another sample of the *same* invariants, not a new code
  path. The genuinely 4d-only C++ coverage is the `CubicGrids` typed tests, which continue to run in
  C++ regardless of the bindings.

## Decision and conditions to revisit

**NO-GO.** The 103-TU instantiation fan-out, the existing binding-TU memory pressure (already
OOM-sensitive, already split by dimension), and the absence of visualization/gmsh make a bound 4d
grid a poor trade in WP2. The 4d code paths remain covered by the C++ `CubicGrids` tests.

Revisit if all of the following hold:

1. A concrete Python use case needs 4d (e.g. a bound space-time or moment-model operator that is
   genuinely 4-dimensional), not merely "more property samples".
2. The measured single-TU peak-RAM delta (recipe below) stays under the ~4 GB guardrail for the
   `spaces/*` and `operators/*` TUs, **or** those TUs are split by dimension first (as
   `interfaces_istl_*` already are) so 4d lands in its own TU set.
3. 4d is introduced behind its own opt-in so a default wheel build is unaffected.

## Measurement recipe (to run in the CI toolchain, not on a bare checkout)

To produce the peak-RAM / build-time delta the guardrail requires, build the heaviest space and
operator binding TUs with and without 4d and diff the numbers:

1. Baseline: build with the current `bindings::AvailableGridTypes`.
2. Add `YASP_4D_EQUIDISTANT_OFFSET` to `Available*GridTypes` (a 4d list; 4d is neither 2d nor 3d) in
   `python/xt/dune/xt/grid/grids.bindings.hh` and bind `make_cube_grid<YASP_4D_EQUIDISTANT_OFFSET,
   Cube>` in `gridprovider/cube.cc`, on a throwaway branch.
3. For the heaviest offenders — e.g. `spaces/l2/discontinuous-lagrange.cc`,
   `spaces/h1/continuous-lagrange.cc`, `operators/matrix-based.cc` and the `operators/interfaces_*`
   set — compile a single object with peak-RSS tracking, e.g.
   `/usr/bin/time -v cmake --build <preset> --target <obj> -- -j1` (read "Maximum resident set size")
   or `ninja -j1` under a memory sampler, both before and after step 2.
4. Report, per TU, ΔpeakRSS and Δwall-clock, and flag every TU whose absolute peak crosses ~4 GB.

If step 4 shows any `spaces/*` or `operators/*` TU crossing ~4 GB (the likely outcome), the go path
mandates a by-dimension TU split for that TU before 4d can be merged.
