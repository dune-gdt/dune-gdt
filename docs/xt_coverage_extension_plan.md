# `dune/xt` test-coverage extension plan

Planning document for the next unit-test work package targeting the `dune/xt`
subtree. Priorities follow the agreed rules for this package:

1. **Target files with 0 % coverage first.**
2. **Then the files with the most _missed_ lines.**
3. **Do not** spend effort on reducing _partials_ (branch coverage) in this package.
4. **Prefer** driving new tests from the existing Python bindings with
   [Hypothesis](https://hypothesis.readthedocs.io/) inputs, modelled on
   `python/xt/test/test_hypothesis_la_eigensolver.py`.
5. Fall back to a **native C++ test** only where no binding exposes the code.
6. **Do not add new bindings** in this package.

## Data source

Numbers are the Codecov report tree for `dune/xt` as surfaced on
[PR #329](https://github.com/dune-gdt/dune-gdt/pull/329) (Codecov v2 API,
`report/tree?path=dune/xt`). Snapshot totals for the subtree:

| Subtree            | Lines  | Hit    | Missed | Partial | Coverage |
|--------------------|-------:|-------:|-------:|--------:|---------:|
| `dune/xt` (total)  | 16 827 | 11 859 | 4 414  | 554     | 70.5 %   |
| `dune/xt/la`       |  4 542 |  2 479 | 1 867  | 196     | 54.6 %   |
| `dune/xt/common`   |  4 383 |  3 014 | 1 189  | 180     | 68.8 %   |
| `dune/xt/functions`|  1 971 |  1 093 |   796  |  82     | 55.5 %   |
| `dune/xt/grid`     |  2 357 |  1 987 |   294  |  76     | 84.3 %   |
| `dune/xt/test`     |  3 574 |  3 286 |   268  |  20     | 91.9 %   |

`dune/xt/la` is by far the largest reservoir of missed lines, and — crucially —
the LA solver machinery is already fully wired into the Python bindings, so most
of it is reachable **without adding a single binding**.

## Key enabling observation

The LA bindings (`python/xt/dune/xt/la/bindings.cc`) already expose, for the
matrix classes each build instantiates:

- `bind_Solver`            → `<Matrix>Solver`            (linear solve `A x = b`)
- `bind_EigenSolver`       → `<Matrix>EigenSolver`       (**already tested**)
- `bind_GeneralizedEigenSolver` → `<Matrix>GeneralizedEigenSolver`
- `bind_MatrixInverter`    → `<Matrix>MatrixInverter`

and `python/xt/dune/xt/test/hypothesis_strategies.py` already ships the
discovery helpers `discover_generalized_eigen_solver_types()` and
`discover_matrix_inverter_types()` — **but only the plain `EigenSolver` has a
test**. The generalized eigen-solver, matrix-inverter and plain linear-solver
code paths are entirely (or almost entirely) uncovered despite being fully bound.

This makes the highest-value 0 %-coverage block reachable purely by adding
Hypothesis test files plus, at most, a one-line strategy helper — no bindings.

---

## Tier 1 — 0 % coverage, reachable via existing Python bindings

These are the primary deliverables. Each is a new Hypothesis test file under
`python/xt/test/`, modelled on `test_hypothesis_la_eigensolver.py`
(binding-discovery + property comparison against NumPy/SciPy, skipped cleanly
when a build binds nothing).

### 1.1 Generalized eigen-solver — **largest single 0 % block (~361 lines)**

| File | Missed | Cov |
|---|---:|---:|
| `dune/xt/la/generalized-eigen-solver/internal/base.hh` | 316 | 0 % |
| `dune/xt/la/generalized-eigen-solver/default.hh`       |  39 | 0 % |
| `dune/xt/la/generalized-eigen-solver.hh`               |   6 | 0 % |

- **Binding:** `bind_GeneralizedEigenSolver` for `CommonDenseMatrix<double>`,
  `CommonSparseMatrixCsr<double>`, `EigenDenseMatrix<double>`.
- **Discovery helper:** `discover_generalized_eigen_solver_types()` — already exists.
- **New file:** `python/xt/test/test_hypothesis_la_generalized_eigensolver.py`.
- **Property:** for a generalized problem `A x = λ B x` with SPD `B`, compare
  eigenvalues against `scipy.linalg.eigvals(A, B)` (or `eigvalsh` for
  symmetric `A`, SPD `B`). Reuse `spd_matrices()` / `make_matrix()` /
  `dense_sparsity_pattern()` from the existing eigensolver test.
- Follow the same guardrails the eigensolver test documents: pin a pure-C++
  solver `type`, disable eigenvector computation and the eigendecomposition
  post-check where they are irrelevant, and generate `1×1` matrices to exercise
  the small-size edge paths.
- **Note:** `generalized-eigen-solver/internal/lapacke.hh` (66 missed) is a
  LAPACKE backend; it will only move if the coverage build links LAPACKE (see
  “Out of scope” below). The `base.hh`/`default.hh` C++ fallback is the target.

### 1.2 Plain linear solver `A x = b` — **~99 lines at 0 %, biggest missed-line lever**

| File | Missed | Cov |
|---|---:|---:|
| `dune/xt/la/solver/dense.hh`     | 44 | 0 % |
| `dune/xt/la/solver/istl/amg.hh`  | 55 | 0 % |
| `dune/xt/la/solver/eigen.hh`     | 135 | 41 % (incidental) |
| `dune/xt/la/solver/istl.hh`      | 58 | 62 % (incidental) |

- **Binding:** `bind_Solver` for `CommonDenseMatrix<double>`,
  `IstlRowMajorSparseMatrix<double>`, `EigenDenseMatrix<double>`,
  `EigenRowMajorSparseMatrix<double>`. Classes are named `<Matrix>Solver`,
  with `.types()`, `.options(type)`, `.apply(rhs, solution)`.
- **Strategy helper needed (not a binding):** add
  `discover_solver_types() = _discover_solver_machinery_types("Solver")` to
  `hypothesis_strategies.py` — one line, reusing the existing private helper.
- **New file:** `python/xt/test/test_hypothesis_la_solver.py`.
- **Property:** build SPD `A` and random `b`, solve with each bound solver
  `type` (`.types()` enumerates them — this is what reaches `dense.hh`, the
  ISTL `amg.hh`/direct branches, and the Eigen solver branches), assert
  `A x ≈ b` and `x ≈ numpy.linalg.solve(A, b)`.
- Iterating over `.types()` per matrix class is what turns the four bound
  solvers into coverage across `dense.hh`, `istl/amg.hh`, `istl.hh`, `eigen.hh`.

### 1.3 Matrix inverter — 0 % backend branch + partial lift

| File | Missed | Cov |
|---|---:|---:|
| `dune/xt/la/matrix-inverter/internal/eigen.hh` | 11 | 0 % |
| `dune/xt/la/matrix-inverter/internal/base.hh`  | 16 | 77 % (incidental) |
| `dune/xt/la/matrix-inverter/fmatrix.hh`        | 14 | 71 % (incidental) |

- **Binding:** `bind_MatrixInverter` for the Common/Eigen dense + CSR classes
  (real and complex). **Discovery helper exists.**
- **New file:** `python/xt/test/test_hypothesis_la_matrix_inverter.py`.
- **Property:** invert well-conditioned matrices (e.g. SPD, or `A + nI`) via
  each `type`, assert `A · A⁻¹ ≈ I` and agreement with `numpy.linalg.inv`.

### 1.4 Grid boundary-detector functor — 35 lines at 0 %

| File | Missed | Cov |
|---|---:|---:|
| `dune/xt/grid/functors/boundary-detector.hh` | 35 | 0 % |

- **Binding:** `python/xt/dune/xt/grid/functors/boundary-detector.cc` binds the
  functor; a `Walker` runs it (`python/xt/test/grid_walker.py` shows the pattern).
- **Approach:** extend the existing Python grid-walker test (or add a Hypothesis
  variant over `grid_specs()` from the strategies module) that attaches a
  boundary-detector functor, walks a generated grid, and asserts the detected
  boundary-intersection count matches the geometry.

---

## Tier 2 — 0 % coverage with **no binding** → native C++ tests

Only where Tier-1 cannot reach. Add `TYPED_TEST`-style native tests under
`dune/xt/test/la/` and `dune/xt/test/functions/`.

| File | Missed | Cov | Why native |
|---|---:|---:|---|
| `dune/xt/la/container/vector-view.hh`   | 114 | 0 % | `VectorView` is not exposed by any binding (no `view` symbol in `python/xt/dune/xt/la`). |
| `dune/xt/functions/base/transformed.hh` |  43 | 0 % | `TransformedGridFunction` / `TransformedFunction` is not bound. |

- `vector-view.hh` is the single largest native-only 0 % file — a focused test
  constructing views over a bound vector type and exercising read/write,
  size, and arithmetic access is worth ~114 lines.
- `transformed.hh` pairs naturally with the existing
  `dune/xt/test/functions/*_operators.cc` tests.

---

## Tier 3 — “most missed” partial files, lifted incidentally (no separate work)

Per rule 3 we do **not** chase partials directly, but the Tier-1 LA tests pass
through the interface-heavy files below, so their missed **lines** (not just
partials) drop for free. Listed for awareness, not as separate deliverables:

| File | Missed | Cov | Lifted by |
|---|---:|---:|---|
| `dune/xt/la/eigen-solver/internal/base.hh` | 188 | 41 % | 1.1 + existing eigensolver test |
| `dune/xt/la/container/vector-interface.hh` | 122 | 49 % | all LA tests |
| `dune/xt/la/container/matrix-interface.hh` | 107 | 51 % | all LA tests |
| `dune/xt/la/container/istl.hh`             |  82 | 63 % | 1.2 (ISTL solver) |
| `dune/xt/la/container/common/matrix/dense.hh` | 72 | 51 % | 1.2 (Common solver) |
| `dune/xt/la/container/eigen/sparse.hh`     |  65 | 56 % | 1.2/1.3 (Eigen matrices) |

### Optional Tier-3 extension — functions interfaces (large missed block)

If the package has room after LA, the `functions` interfaces are the next
biggest missed reservoir and are Python-reachable (constant/expression/generic
function bindings exist, and the strategies module already ships `polynomials`,
`fourier_sums`, and `has_generic_function`):

| File | Missed | Cov |
|---|---:|---:|
| `dune/xt/functions/interfaces/element-functions.hh`      | 169 | 21 % |
| `dune/xt/functions/interfaces/element-flux-functions.hh` | 129 |  5 % |
| `dune/xt/functions/base/combined.hh`                     |  78 | 35 % |
| `dune/xt/functions/interfaces/function.hh`               |  60 | 30 % |
| `dune/xt/functions/generic/element-function.hh`          |  59 | 43 % |

A `test_hypothesis_functions.py` evaluating `evaluate`/`jacobian` of generic and
expression functions against the analytic `Polynomial`/`FourierSum` references
already defined in `hypothesis_strategies.py` would exercise these. `element-flux-functions.hh`
(5 %, nearly zero) is the strongest candidate here.

---

## Explicitly out of scope for this package

These show as low/0 % coverage but **will not move by adding tests** — flag, do
not target:

- **Backend wrappers, uncovered because the backend is not linked in the
  coverage build**, not because tests are missing:
  `common/lapacke.cc` (125), `common/cblas.cc` (70), `common/mkl.cc` (9),
  `eigen-solver/internal/lapacke.hh`, `eigen-solver/internal/numpy.hh`,
  `generalized-eigen-solver/internal/lapacke.hh` (66). Moving these is a
  **build-configuration** change (link LAPACKE/MKL/CBLAS in the coverage
  preset), tracked separately from test authoring.
- **Existing C++ tests that are compiled but not executed in the coverage run**
  (they report 0 % as _test_ files, so writing “new tests” is the wrong lever):
  `dune/xt/test/grid/walker.cc` (156), `dune/xt/test/common/parallel.cc` (60).
  Investigate why the coverage CTest run skips them (label/`MPI`/timeout).
- **Partials-only / small C++ utility files** already covered by native tests
  (`common/configuration.cc`, `common/print.hh`, `common/timedlogging.cc`,
  `common/string_internal.hh`, `common/signals.cc`, …): excluded by rule 3.

---

## Suggested execution order

| Step | Deliverable | Kind | Approx. 0 % lines targeted |
|---|---|---|---:|
| 1 | `test_hypothesis_la_generalized_eigensolver.py` | Python + Hypothesis | ~361 |
| 2 | `discover_solver_types()` helper + `test_hypothesis_la_solver.py` | Python + Hypothesis | ~99 (+ ~193 partial lift) |
| 3 | `test_hypothesis_la_matrix_inverter.py` | Python + Hypothesis | ~11 (+ ~30 partial lift) |
| 4 | boundary-detector coverage in the grid-walker Python test | Python + Hypothesis | ~35 |
| 5 | native `vector-view` test (`dune/xt/test/la/`) | C++ | ~114 |
| 6 | native `transformed` function test (`dune/xt/test/functions/`) | C++ | ~43 |
| 7 | _(optional)_ `test_hypothesis_functions.py` | Python + Hypothesis | interfaces block |

Steps 1–4 are pure Python/Hypothesis on already-bound code — no new bindings,
matching the package rules — and cover the largest 0 % block (LA solver
machinery). Steps 5–6 are the only native additions, reserved for the two 0 %
files with no binding.
