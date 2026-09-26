```
# This file is part of the dune-gdt project:
#   https://github.com/dune-gdt/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
```

# Adversarial review of the PDE-solver core (C++)

*Scope: the directly PDE-solver-relevant C++ code — the assembly pipeline
(`dune/gdt/local/*`, `dune/gdt/operators/*`), spaces/bases/mappers/discrete functions
(`dune/gdt/spaces/*`, `dune/gdt/discretefunction/*`, `dune/gdt/local/finite-elements/*`),
the vendored support layer (`dune/xt/la`, `dune/xt/grid`, `dune/xt/functions`), time stepping
(`dune/gdt/tools/timestepper/*`) and the hyperbolic/FV machinery
(`dune/gdt/local/numerical-fluxes/*`, `dune/gdt/operators/reconstruction/*`).*

*Method: three independent exploration passes plus first-hand verification of every
high-severity claim against source. Findings marked "(reported)" were surfaced by an
exploration pass and are cited with file/line but were not independently re-derived;
everything else was read and confirmed directly. Line numbers refer to the current `main`.*

---

## Executive summary

The library is a carefully layered, very generic discretization toolbox — and that generality
has been bought at the direct expense of the two things a PDE solver must do well: run the
element loop fast and scale across threads. The three most damaging structural decisions:

1. **The innermost loop is virtual and type-erased.** Every quadrature-point evaluation goes
   through a pure-virtual `evaluate()` on a heap-cloned integrand. Nothing in the hot path can
   be inlined or vectorized across the abstraction boundary.
2. **Thread safety is implemented as locking around scalar writes**, in three inconsistent
   layers (striped container mutexes, one program-wide `static std::mutex`, and several
   admitted-broken or unused locks) — so parallel assembly serializes where it matters and
   races where it doesn't lock.
3. **"Local" view objects are secretly O(N).** Creating a local discrete function deep-copies
   the entire space, which rebuilds the DoF mapper over the whole grid.

In addition, the review found **several outright correctness bugs** in shipping code paths,
including two `=`-vs-`+=` accumulation bugs (L² interpolation and `restrict_to`), a numerical
flux whose function bodies are entirely commented out (undefined behavior if called), and a
coupling sparsity pattern that dereferences boundary neighbors.

Section D proposes a re-architecture path that keeps the public API style while fixing the
cost model: a statically dispatched assembly kernel, thread-local scatter instead of locked
containers, a reference-quadrature shape-value cache, and borrow-not-copy local views.

---

## A. Architectural findings (cross-cutting)

### A1. Virtual dispatch + type erasure in the quadrature loop

The integrand interfaces (`dune/gdt/local/integrands/interfaces.hh:131, 257, 516, 649`)
declare `evaluate()` as pure virtual, and the integral bilinear forms call it **once per
quadrature point** through a `std::unique_ptr<IntegrandInterface>` obtained from
`copy_as_binary_element_integrand()` (`dune/gdt/local/bilinear-forms/integrals.hh:63, 131`):

```cpp
for (auto [point_in_reference_element, quadrature_weight] : QuadratureRules<D, d>::rule(...)) {
  const auto factor = element.geometry().integrationElement(point_in_reference_element) * ...;
  integrand_->evaluate(test_basis, ansatz_basis, point_in_reference_element, integrand_values_, param);
  ...
}
```

Because the integrand is type-erased, the compiler cannot inline, hoist, or vectorize across
this call — for a P1 Laplace assembly the virtual dispatch and the un-inlinable basis access
dominate the (tiny) arithmetic payload. Summed integrands (`operator+`,
`local/integrands/combined.hh:234-250`) double the dispatch and add a full `rows×cols` matrix
add per point; nesting sums multiplies it. `GenericLocal*Integrand`
(`local/integrands/generic.hh:106-197`) stores `std::function` members invoked per point —
a second indirection layer. There is **no static-dispatch path at all**: even a user who knows
their integrand type at compile time is forced through the virtual interface.

The same erasure applies one level up (local bilinear forms cloned via `copy()`), and one level
down (`ElementFunctionSetInterface` for the bases).

### A2. The DoF-write path is built out of mutexes

The XT::LA containers guard scalar writes with striped mutexes
(`dune/xt/la/container/container-interface.hh:35-72`):

- `add_to_entry` takes a `LockGuard` per scalar write (`dune/xt/la/container/istl.hh:267-272`;
  matrix variant `istl.hh:625-630`).
- `axpy`, `scal`, `iadd`, `isub` lock **all** mutexes of the container via `boost::lock`
  (`istl.hh:232, 241, 340, 349`) on every call — including in single-threaded use.
- Containers default to `num_mutexes = 1` (`istl.hh:113`), and the `make_matrix_operator`
  factories construct assembly matrices with exactly that default
  (`dune/gdt/operators/matrix.hh:680, 712, 767, 805`).

Consequence: under `assemble(/*use_tbb=*/true)`, every thread's per-entry scatter into the
global matrix serializes through **one** mutex — the parallel walk parallelizes the local
integration and then funnels all writes through a global lock. Single-threaded runs pay a
lock/unlock per matrix entry for nothing.

It gets worse at the next layer up. `LocalDofVector::operator[]`
(`dune/gdt/local/dof-vector.hh:299-306`):

```cpp
ScalarType& operator[](const size_t ii)
{
  static std::mutex mutex;
  [[maybe_unused]] std::lock_guard<std::mutex> guard{mutex};
  ...
}
```

A **function-local `static` mutex shared program-wide** — every mutable DoF access through
`operator[]`, on any vector, in any thread, takes the same lock. The comment admits: *"The
mutex here is just a quick fix, properly fix thread safety."* Meanwhile `set_entry` /
`add_to_entry` / `get_unchecked_ref` on the same class (`:258-292`) take **no** lock, so the
protection is simultaneously catastrophic for scaling and illusory for safety.

### A3. "Cheap" local views are O(N)

`ConstLocalDiscreteFunction`'s constructor (`dune/gdt/local/discretefunction.hh:71-81`) does
`space_(spc.copy())`. For the Lagrange spaces, `copy()` invokes the copy constructor, which
**calls `update_after_adapt()`** (`dune/gdt/spaces/h1/continuous-lagrange.hh:87-96`,
`l2/discontinuous-lagrange.hh:94-104`) — rebuilding the mapper by iterating the entire grid
view and allocating a fresh finite-element family (discarding the existing FE cache).

So every `local_function()` / `local_discrete_function()` call is **O(#grid elements)**. Any
code that (reasonably) creates a local function per element — and the API invites exactly
that — is silently O(N²). A read-only per-element view has no business owning a copy of the
space at all.

### A4. Logging and parameter plumbing woven through hot classes

Every integrand and bilinear form inherits `ParametricInterface` + `ElementBoundObject` +
`WithLogger<...>` (two `std::string`s plus logger state per object), and these objects are
deep-cloned per thread and per assembler copy. The `LOG_(...)` macro is correctly guarded
(`dune/xt/common/timedlogging.hh:153-155` — arguments are *not* evaluated when disabled,
contrary to what one might fear), but the guard branch sits inside quadrature loops
(`local/bilinear-forms/integrals.hh:127`) and constructors of hot-cloned objects, and the
logger strings are copied on every clone. Infrastructure concerns (logging, string-keyed
parameters like `"matrixoperator.scaling"` at `local/assembler/bilinear-form-assemblers.hh:71`)
are baked into the numerical core instead of layered around it.

### A5. Copy-paste parallel hierarchies

- Three near-identical bilinear-form assemblers (element / coupling-intersection /
  intersection, `local/assembler/bilinear-form-assemblers.hh` — ~430 lines that are one
  class in triplicate), mirrored again by the functional assemblers.
- Three near-identical functor wrappers in the walker (`dune/xt/grid/walker.hh:64-196`).
- The space classes are near-verbatim clones of each other (ctors, copy ctors, `copy()`,
  accessor boilerplate in `h1/continuous-lagrange.hh` vs `l2/discontinuous-lagrange.hh` vs
  `hdiv/raviart-thomas.hh` vs `l2/finite-volume.hh`) with no shared implementation base.
- Two duplicated Butcher-tableau provider families (`tools/timestepper/explicit-rungekutta.hh:29-144`
  vs `adaptive-rungekutta.hh:39-142`).
- `operators/matrix.hh` holds five `std::list<std::tuple<unique_ptr<...>, unique_ptr<...>>>`
  members with copy-pasted iteration lambdas and standing TODOs ("Use std::pair instead of
  std::tuple!", `:287`).

Every fix in these areas must be applied three to six times; divergence is already visible
(e.g. logging TODOs at `matrix.hh:284-286`).

### A6. Strings, Configuration, and exceptions as control flow

- Solver/inverse selection is string-keyed `Configuration` dispatch throughout
  (`operators/interfaces.hh:589-676`, `xt/la/solver/istl.hh:248-325`).
- `ConstMatrixOperator::apply_inverse` catches `configuration_error` to decide "the option may
  be one from the base class" (`operators/matrix.hh:206-224`).
- `BilinearFormInterface::apply2` catches `operator_error` to fall back to a grid-function
  reinterpretation (`operators/interfaces.hh:445-454`).
- `newton_solve` iterates over all linear-solver types **catching
  `linear_solver_failed` as the loop-continue condition** (`algorithms/newton.hh:101-110`),
  and reassembles a fresh Jacobian every Newton iteration with no reuse policy (`:92-110`).

Exceptions on the routine path defeat branch prediction, make profiling misleading, and turn
configuration typos into behavior changes instead of errors.

### A7. The threading story is a patchwork of in-source admissions

Beyond A2, the code documents its own races:

- FE cache: *"TODO: Double checked locking pattern is not thread-safe without memory
  barriers"* — `ThreadSafeDefaultLocalFiniteElementFamily::get()` reads `fes_.count(key)`
  unlocked while writers mutate the map under the lock
  (`local/finite-elements/default.hh:211-225`). Every mapper query and basis bind funnels
  through this.
- Raviart-Thomas FE factory: *"Can't figure out why this lock is needed, but for some reasons
  without it we are not thread-safe"* (`local/finite-elements/raviart-thomas.hh:373-379`).
- Interpolation disables threading for everything except FV: *"Basis functions other than FV
  do not seem to be thread safe. TODO: fix"* (`interpolations/default.hh:131`).
- Shared `mutable` scratch members on conceptually-shared objects: numerical-flux state
  buffers (`local/numerical-fluxes/interface.hh:140-187`), basis interpolation scratch
  (`spaces/basis/interface.hh:129-130`), local-FE scratch
  (`local/finite-elements/interfaces.hh:158-159`).
- `ThreadResultPropagator::finalize_imp()` does an unsynchronized read-modify-write into the
  shared base functor; the class declares a `std::mutex mutex_` that is **never locked
  anywhere** (`dune/xt/common/parallel/threadstorage.hh:402-411`).

The per-thread-copy model (integrands note: *"the integrand is copied for each thread"*) is a
sound design — but it is enforced purely by convention, and the layers underneath it violate
it.

---

## B. Correctness findings

Ordered by severity. B1–B5 verified by direct read; B6–B10 reported with citations.

1. **L² interpolation keeps only the last quadrature point.**
   `LocalL2FiniteElementInterpolation::interpolate`
   (`local/finite-elements/default.hh:287-294`):
   ```cpp
   for (auto&& quadrature_point : QuadratureRules<D, d>::rule(...)) {
     ...
     for (size_t ii = 0; ii < local_basis_->size(); ++ii)
       dofs[ii] = (basis_values[ii] * function_value) * quadrature_weight;   // '=' not '+='
   }
   ```
   The quadrature sum is overwritten each iteration; the computed "integral" is just the last
   point's contribution. (Independently: this is only the RHS of an L² projection with no
   mass-matrix solve, so it is exact only for orthonormal bases even after the fix.)

2. **`SpaceInterface::restrict_to` assigns every DoF the last basis value.**
   The functional integrand lambda (`spaces/interface.hh:202-207`):
   ```cpp
   for (size_t ii = 0; ii < basis.size(); ++ii)
     result = child_basis_values[ii] * restriction_data_value;   // assigns the whole DynamicVector
   ```
   `result` is the `DynamicVector<F>&` of *all* basis integrand values; assigning a scalar
   sets every entry, and the loop leaves all entries equal to the `ii = basis.size()-1` value.
   The restriction data feeding grid adaptation is wrong.

3. **The Vijayasundaram flux is shipped as undefined behavior.**
   Both `default_flux_eigen_decomposition()` and `apply()` have their entire bodies commented
   out under `//! TODO: re-enable` (`local/numerical-fluxes/vijayasundaram.hh:59-75, 115-139`)
   — non-void functions with no `return`, still constructible through
   `make_numerical_vijayasundaram_flux`. Control flows off the end of a non-void function:
   UB, likely garbage or a crash, with no compile-time or run-time guard.

4. **`ConstDiscreteFunction::copy_as_grid_function()` cannot compile if used.**
   `return std::unique_ptr<ThisType>(*this);` (`discretefunction/default.hh:119-122`) tries
   to construct a `unique_ptr` from an lvalue object. Dead-but-armed code; the correct sibling
   `copy_as_discrete_function()` (`:114-117`) shows the intent.

5. **Continuous mapper is known-wrong for order ≥ 3 on simplices.**
   The local→global map assumes consistent sub-entity DoF ordering across elements; the
   in-source TODO documents that simplex grids violate this
   (`spaces/mapper/continuous.hh:114-124`), and the constructor papers over it with a
   grid-type whitelist throw (`:67-71`) — plus a `DEBUG_THROW_IF` that vanishes in release
   builds for mixed grids (`:121`). High-order CG is silently unsupported territory.

6. **Coupling sparsity pattern dereferences boundary "neighbors".** (reported)
   `make_coupling_sparsity_pattern` calls `intersection.outside()` with no
   `intersection.neighbor()` guard (`tools/sparsity-pattern.hh:219-220`) — invalid on boundary
   intersections (contrast the guarded `make_intersection_sparsity_pattern`, `:88`). The in/in
   block also uses `test_space.mapper().local_size(inside)` for **both** loop bounds
   (`:223-225`), which is wrong whenever test and ansatz spaces differ.

7. **Timestepper trajectory storage: float-keyed map, stores everything by default.**
   The solution trajectory is `std::map<RangeFieldType, unique_ptr<DiscreteFunction>, FloatCmpLt>`
   (`tools/timestepper/interface.hh:60-66`) and the default `num_save_steps = size_t(-1)`
   deep-copies the full DoF vector at **every step** (`:224-262`) — O(#steps × #DoFs) memory
   growth by default, with float-comparison map lookups as a bonus hazard.

8. **Adaptive RK rolls back by re-subtraction.** (reported)
   A rejected step is undone by adding the negated stage updates
   (`tools/timestepper/adaptive-rungekutta.hh:333, 346-348`) instead of restoring a saved
   copy — floating-point non-associativity perturbs `u_n` on every rejection.

9. **`MatrixOperator` silently ignores appended intersection FD-jacobian operators.** (reported)
   The apply path for `intersection_fd_operator_data_` is commented out with *"TODO: fix the
   following line ... the intersection type here is wrong!"* (`operators/matrix.hh:462-464`)
   — appending such an operator succeeds and then does nothing.

10. **`SlopeBase::get` and `get_char` are mutually recursive defaults.** (reported)
    The contract "at least one ... has to be implemented" is only a comment
    (`operators/reconstruction/slopes.hh:44-66`); a subclass overriding neither compiles and
    stack-overflows at runtime.

Also noted: `DiscontinuousMapper::size()` accumulates `size_t` sizes into an `int` literal
(`std::accumulate(size_.begin(), size_.end(), 0)`, `spaces/mapper/discontinuous.hh:95`) —
truncation past 2³¹ DoFs; `DummyEigenVectorWrapper::eigenvectors()` returns a reference to a
dummy after a `DUNE_THROW` (`operators/reconstruction/internal.hh:166-173`).

---

## C. Performance findings (hot paths)

1. **Geometry re-fetched per quadrature point.** `element.geometry().integrationElement(...)`
   inside the quadrature loop (`local/bilinear-forms/integrals.hh:125`); `geometry()` is not
   free on all grid implementations and belongs outside the loop (as does the geometry cache
   per bind).

2. **Default dynamic-size function evaluation heap-allocates per call.**
   `ElementFunctionSetInterface`'s dynamic `evaluate`/`jacobian`/`derivative` overloads do
   `std::make_unique<std::vector<RangeType>>(1)` **per invocation**
   (`dune/xt/functions/interfaces/element-functions.hh:591, 604, 618`). Any grid function that
   doesn't override them (generic/combined functions used as coefficients) allocates and frees
   at every quadrature point of every element. `ConstLocalDiscreteFunction` escapes only by
   manually overriding all of them — a fragile per-class opt-out.

3. **No shape-value caching anywhere.** Basis `evaluate` delegates straight to
   dune-localfunctions per point and `jacobians()` re-fetches `jacobianInverseTransposed`
   per point (`spaces/basis/default.hh:149-181`). Shape functions at reference quadrature
   points are identical for every element of a geometry type; recomputing them per element is
   the single largest avoidable arithmetic cost in assembly. (Compare deal.II `FEValues` /
   dune-pdelab's cached local basis.)

4. **Engquist–Osher constructs a Dune grid per flux evaluation.**
   `const OneDGrid state_grid(1, 0., s[0]);` inside `integrate_f`
   (`local/numerical-fluxes/engquist-osher.hh:93`), called **twice per intersection** per
   stage per timestep, plus a by-value `std::function` argument per call — for a 1-D state
   integral that needs a quadrature rule and nothing else.

5. **Explicit RK allocates temporaries per stage.**
   `u_i_->dofs().vector() += stages_k_[jj]->dofs().vector() * (actual_dt * r_ * A_[ii][jj])`
   (`tools/timestepper/explicit-rungekutta.hh:254`, again at `:267`) — `operator*` returns a
   full new vector each time; `axpy` exists and is unused here. Each `+=`/`axpy` additionally
   locks all container mutexes (A2). The LLF flux evaluates both-side flux Jacobians per
   intersection when `lambda` is auto-computed (`lax-friedrichs.hh:79-92`, reported).

6. **Mapper `global_indices` recomputed per bind with virtual calls per DoF.**
   `local_coefficients(type)` goes through the racy FE-cache map lookup, then a loop of
   virtual `local_key(ii)` + `mapper_.subIndex(...)` per DoF
   (`spaces/mapper/continuous.hh:107-140`, `discontinuous.hh:131-153`). Per-geometry-type
   index templates are never cached.

7. **Per-thread functor cloning is heavyweight.** Each thread's assembler copy re-clones the
   *space* (twice — test and ansatz), the bilinear form, the integrand, and re-localizes both
   bases (`local/assembler/bilinear-form-assemblers.hh:95-109`); with A3 this multiplies into
   per-thread O(N) setup. The walker also visits inner intersections twice (once from each
   side, `dune/xt/grid/walker.hh:709-743`) and evaluates virtual filter `contains()` per
   intersection per functor wrapper.

8. **DG mass-matrix application allocates per element.**
   `local_dofs_ = local_mass_matrices_.access().local_mass_matrix_inverse(element()) * local_dofs_;`
   with the in-source note *"not optimal, uses a temporary"*
   (`local/operators/advection-dg.hh:154-156`) — a by-value matrix return plus a temporary
   vector in the per-element hot path.

9. **Reconstruction: LAPACK per element, silent degradation.** (reported)
   `EigenvectorWrapper::compute_eigenvectors_impl` runs `dgeevx` + QR + `dtrcon` per element
   per direction (`operators/reconstruction/internal.hh:246-321`); on `MathError` it silently
   falls back to identity eigenvectors and unit eigenvalues (`:308-316`) — a
   correctness-affecting fallback with no diagnostic. `apply` reallocates a whole-grid
   `std::vector` of cell values per invocation (`linear.hh:402-408`).

10. **ISTL solver `apply` copies the RHS and self-checks by default.** (reported)
    `writable_rhs = rhs.copy()` plus the default `post_check_solves_system = 1e-5` forcing an
    extra `mv` and two more full-vector copies on **every** solve
    (`dune/xt/la/solver/istl.hh:246, 336-341`).

11. **Interface-default vector arithmetic is per-element virtual.** (reported)
    `VectorInterface::add/iadd/axpy` defaults loop scalar `set_entry`/`add_to_entry`
    (`dune/xt/la/container/vector-interface.hh:370-408`); ISTL overrides with backend BLAS but
    other containers (vector views, common/eigen variants) fall through to the slow path.
    `operator+`/`operator-` always `copy()` first (`:512-537`). The `shared_ptr` backend held
    by containers is never actually shared — copy-on-write was removed but the indirection and
    per-assignment mutex reallocation remain (`istl.hh:139-176`).

12. **Library code prints to `std::cout` unconditionally** in `find_suitable_dt` and the
    solve loop (`tools/timestepper/explicit-rungekutta.hh:292-309`,
    `tools/timestepper/interface.hh:278`).

---

## D. Re-architecture opportunities

Ordered by leverage-per-effort; items 1–4 change the cost model, 5–10 are consolidation.

1. **Introduce a statically dispatched assembly kernel.** Make the integrand a *template
   parameter* of the local bilinear form / assembler, and confine type erasure to the API
   boundary (one thin adapter that erases an already-composed, statically-typed kernel).
   The public "compose integrands, append to operator" UX stays; the quadrature loop becomes
   inlinable and vectorizable. This is the single biggest performance lever and it is purely
   additive — the virtual interface can remain as the fallback.

2. **Remove locks from the DoF-write path.** Replace mutex-per-container with either
   (a) per-thread local scatter buffers (COO or row-chunked) merged once at `finalize()`, or
   (b) grid coloring / interior-partition scheduling so no two threads touch the same row.
   Delete the striped-mutex machinery from the containers (making `add_to_entry` a plain
   `+=`), and delete `LocalDofVector::operator[]`'s static mutex outright. Assembly becomes
   embarrassingly parallel; single-threaded use stops paying for threads it doesn't have.

3. **Add a reference-quadrature shape-value cache.** A per-(GeometryType, FE, quadrature
   order) cache of shape values and reference gradients, computed once and shared read-only;
   `bind(element)` then only applies the geometry transform to gradients. This is the standard
   design in deal.II (`FEValues`) and dune-pdelab and typically wins integer factors on
   assembly. It also gives the FE cache a single, properly synchronized home (fixing A7's
   broken double-checked locking by construction — e.g. `std::shared_mutex` or a pre-populated
   immutable map).

4. **Make local views borrow, not copy.** Split spaces into an immutable core (grid view,
   mapper, basis, FE family) held by `shared_ptr`, and thin mutable adapt state.
   `local_function()` should hold a `shared_ptr` to the core — O(1) — instead of `space_.copy()`
   rebuilding the mapper (A3). The same split fixes the per-thread assembler clone cost (C7)
   and makes the `copy()`-per-thread convention cheap enough to be harmless.

5. **Collapse the copy-paste hierarchies.** One generic local assembler parameterized over
   (arity, codimension) replaces the six near-identical assembler classes; one functor wrapper
   with a policy replaces three; a CRTP `SpaceBase` absorbs the duplicated space boilerplate
   (ctor/copy/update_after_adapt/accessors); one Butcher-tableau table serves both RK steppers.

6. **Replace string/Configuration dispatch with typed options; stop using exceptions as
   control flow.** Typed option structs (with a single string→struct parse at the API edge for
   Python), capability queries (`supports_jacobian()`, `invert_types()`) instead of
   try/catch fallbacks in `apply_inverse`/`apply2`/`newton_solve` (A6). Newton should own a
   Jacobian-reuse policy (freeze for k iterations / reuse pattern) instead of reassembling
   every iteration.

7. **Fix the timestepper storage and arithmetic.** Preallocated stage vectors + `axpy` (no
   `operator*` temporaries); an observer/callback (or ring buffer) for solution output instead
   of a default store-every-step float-keyed map; restore-from-copy instead of arithmetic
   rollback in the adaptive stepper; route diagnostics through the logging layer instead of
   `std::cout`.

8. **Clean up the flux layer.** Integrate the Engquist–Osher state integral with a plain
   quadrature rule on [0, s] (no `OneDGrid`); pass comparators as template parameters or
   function pointers, not `std::function`; either finish or delete Vijayasundaram (as shipped
   it is UB — at minimum make the constructor throw `NotImplemented`); make flux objects
   stateless per-evaluation (drop the `mutable` scratch members) so the parallel intersection
   walk is safe by construction.

9. **Ship the small correctness patches now**, independent of any re-architecture: the two
   `=`→`+=` bugs (B1, B2), the `neighbor()` guard and test/ansatz mix-up in
   `make_coupling_sparsity_pattern` (B6), the `int` accumulator in `DiscontinuousMapper::size()`,
   `copy_as_grid_function()` (B4), and a `DUNE_THROW` in the unimplemented Vijayasundaram
   methods (B3). All are hours, not weeks, and several silently corrupt results today.

10. **Attack compile time and header hygiene.** `operators/interfaces.hh` is a 1178-line god
    header mixing the interface with dozens of convenience overloads and pulling in Newton,
    interpolation, and sparsity machinery; split interface/convenience/impl, and use explicit
    template instantiation units for the common (grid × space × container) combinations the
    tests and Python bindings already enumerate.

---

## Closing assessment

The codebase is disciplined about interfaces, naming, and documentation, and its
test/EOC-study culture is real. But the runtime architecture treats the element loop — the
part of a PDE solver that *is* the product — as just another interface boundary. Virtual
dispatch per quadrature point, locked scalar writes, O(N) "local" views, and recomputed shape
functions compound multiplicatively; fixing any one of them in isolation will under-deliver,
which is exactly why the static-kernel + thread-local-scatter + shape-cache trio (D1–D3)
should be planned together. The correctness list (Section B) is short but sharp: two silent
result-corrupting bugs and one UB code path are in shipping headers today and deserve fixes
before any performance work.
