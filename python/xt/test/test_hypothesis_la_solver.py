# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2026 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2026)
# ~~~
"""Property-based tests of the bound LA::Solver (dune/xt/la/solver.hh): x = A^{-1} b.

The compiled counterpart, dune/xt/test/la/solver.tpl, only ever solves against the *identity*
matrix (ContainerFactory::create builds I), so while it iterates Solver::types() it never actually
exercises the numeric solve branches in solver/{eigen,istl,common}.hh -- with A = I every backend
trivially returns the rhs. Here hypothesis supplies genuine, well-conditioned SPD systems, so each
type in Solver::types() runs its real factorization/iteration path, and the result is checked by its
residual (the meaningful, solution-unique-for-SPD property) rather than against a numpy solve whose
elimination order differs.

Iterating the full Solver::types() list is safe by construction: types() already returns only the
solver types valid for that matrix backend, exactly as solver.tpl relies on.

Beyond the default and per-type residual checks, three further properties widen the reached
branches: an x ~ numpy.linalg.solve(A, b) comparison using an explicit direct solver type (the
issue's stated acceptance criterion); a per-type sweep with the internal post-solve checks disabled, which
reaches the "check off" branches of solver/{common,eigen,istl}.hh that the always-enabled defaults
never do; and a single Solver reused across several right-hand sides, guarding the matrix-reference
lifetime (the keep_alive fix below) past the one apply() the other tests make.

Found and fixed a real bug: python/xt/dune/xt/la/solver.hh's `bind_Solver` bound the Solver(matrix)
constructor as plain `py::init<M>()`, with no `py::keep_alive`. `Solver<M>` stores its matrix by
const reference (`const MatrixType& matrix_;` in solver/{common,eigen,istl}.hh), so
`solver_for(cls)(make_matrix(cls, a))` -- constructing from a matrix temporary with no other Python
reference, exactly the pattern below -- left `matrix_` dangling once the constructor call
completed, and any later apply() then read/wrote through freed memory (a segfault, always on the
first call into apply() regardless of backend).

The first attempted fix, adding plain `py::keep_alive<1, 2>()` to the existing
`py::init<M>()`, was NOT sufficient and the crash reproduced identically: `py::init<M>()` takes its
argument by *value* (M, not `const M&`), so pybind11's generated init factory copies the
Python-owned matrix into a temporary and passes that temporary into Solver's constructor -- matrix_
then binds to the temporary, which is destroyed as soon as the init factory call returns, before any
keep_alive relationship (which only ties *Python object* lifetimes together, and never applied to a
temporary with no Python wrapper of its own) can matter. The actual fix replaces `py::init<M>()` with
`py::init([](const M& matrix) { return std::make_unique<C>(matrix); })`: the lambda's `const M&`
parameter makes the type caster hand back a reference to the real Python-owned instance (no copy),
so matrix_ binds to that instance directly, and `py::keep_alive<1, 2>()` then correctly keeps it
alive for as long as the solver lives. The sibling `make_solver` factory in the same file already
used this reference-taking-lambda pattern (with `py::keep_alive<0, 1>`), as did the
EigenSolver/MatrixInverter constructors (solver_machinery.hh's bind_single_matrix_solver_ctor, with
`py::keep_alive<1, 2>`) -- bind_Solver's own constructor was the one binding using plain
`py::init<M>()` instead. No test-side workaround needed.
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    dense_sparsity_pattern,
    discover_solver_types,
    discover_vector_types,
)

MATRIX_CLASSES = list(discover_solver_types())
VECTOR_CLASSES = list(discover_vector_types(field="double"))

# residual tolerance relative to ||b||: the iterative istl/eigen backends stop at their own default
# tolerance, looser than a direct solve, so this is deliberately generous
RTOL = 1e-6


def solver_for(matrix_cls):
    import dune.xt.la as la

    return getattr(la, matrix_cls.__name__ + "Solver")


def matching_vector_class(matrix_cls):
    name = matrix_cls.__name__
    for vector_cls in VECTOR_CLASSES:
        prefix = vector_cls.__name__[: -len("Vector")]
        if name.startswith(prefix):
            return vector_cls
    raise AssertionError(f"no double vector class found for {name}")


def make_matrix(cls, arr):
    rows, cols = arr.shape
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    for ii in range(rows):
        for jj in range(cols):
            mat.set_entry(ii, jj, float(arr[ii, jj]))
    return mat


def make_vector(cls, values):
    vec = cls(len(values), 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def as_numpy(vec, n):
    return np.array([vec.get_entry(ii) for ii in range(n)])


@st.composite
def spd_system(draw, min_size=1, max_size=6, bound=4.0):
    """A well-conditioned SPD matrix A = B^T B + n*I and a bounded rhs b of matching size.

    The modest entry bound keeps the condition number low enough that every iterative backend in
    Solver::types() reaches its stopping criterion; the +n*I floor keeps the smallest eigenvalue >=1.
    """
    n = draw(st.integers(min_size, max_size))
    entries = draw(
        st.lists(
            st.floats(-bound, bound, allow_nan=False, allow_infinity=False),
            min_size=n * n,
            max_size=n * n,
        )
    )
    b = np.asarray(entries).reshape(n, n)
    a = b.T @ b + n * np.eye(n)
    rhs = np.asarray(
        draw(
            st.lists(
                st.floats(-bound, bound, allow_nan=False, allow_infinity=False),
                min_size=n,
                max_size=n,
            )
        )
    )
    return a, rhs


def assert_solves(a, rhs, x, context, tol=RTOL):
    residual = a @ x - rhs
    scale = float(np.linalg.norm(rhs)) + 1.0
    assert float(np.linalg.norm(residual)) <= tol * scale, (
        f"{context}: residual too large"
    )


def direct_solver_types(solver_cls):
    """The direct (non-iterative) types among solver_cls.types().

    A direct factorization solves to ~machine precision, so its solution vector may be compared to
    numpy.linalg.solve directly; the iterative "bicgstab.*" / "cg.*" families instead stop at their
    own residual criterion (coupled to cond(A)), which is why test_every_solver_type_solves checks
    those by residual rather than against a solution vector. Direct types are identified by *not*
    belonging to an iterative family -- a negative prefix test, which (unlike a positive substring
    match on "lu"/"qr"/...) does not misclassify names like "bicgstab.ilut" or "bicgstab.amg.ilu0",
    whose ILU(T) preconditioner spelling happens to contain "lu". A backend may expose no direct type
    at all (ISTL's list is all-iterative unless the build compiled SuperLU/UMFPACK).
    """
    return [tp for tp in solver_cls.types() if not (tp.startswith(("bicgstab", "cg")))]


def options_with_checks_disabled(solver_cls, solver_type):
    """The default options for solver_type, with every optional internal correctness check off.

    Both post-solve checks default to *enabled* in Solver::options(): "post_check_solves_system"
    (all backends, "1e-5") and "check_for_inf_nan" (the Eigen backend, "1"). Every default-apply
    path above therefore only ever runs the "check is on" branch of solver/{common,eigen,istl}.hh
    -- and so does solver.tpl. Setting them to a non-positive value drives the complementary
    "check disabled" branch instead. Keys a given backend does not have are left untouched, so the
    same helper works uniformly across Common/Istl/Eigen.
    """
    opts = dict(solver_cls.options(solver_type))
    opts["post_check_solves_system"] = "-1"
    if "check_for_inf_nan" in opts:
        opts["check_for_inf_nan"] = "0"
    return opts


@pytest.mark.skipif(not MATRIX_CLASSES, reason="no LA solver binding available")
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestSolverResidual:
    @settings(deadline=None)
    @given(system=spd_system())
    def test_default_apply_solves(self, cls, system):
        a, rhs = system
        n = len(rhs)
        vec_cls = matching_vector_class(cls)
        solver = solver_for(cls)(make_matrix(cls, a))
        solution = make_vector(vec_cls, [0.0] * n)
        solver.apply(make_vector(vec_cls, rhs), solution)
        # the residual is the condition-number-independent correctness property; comparing the
        # solution vector directly to np.linalg.solve would couple the tolerance to cond(A) and to
        # whichever (possibly iterative) type Solver picks by default, so we don't.
        assert_solves(a, rhs, as_numpy(solution, n), "default apply")

    @settings(deadline=None)
    @given(system=spd_system())
    def test_every_solver_type_solves(self, cls, system):
        a, rhs = system
        n = len(rhs)
        vec_cls = matching_vector_class(cls)
        solver_cls = solver_for(cls)
        solver = solver_cls(make_matrix(cls, a))

        types = solver_cls.types()
        assert types, "Solver advertises no types"
        # per-type residual tolerance is looser than the default-apply check above: the iterative
        # istl/eigen types stop at their own (type-dependent) default tolerance, and this loop's job
        # is to drive every dispatch branch and confirm each type genuinely solves (a broken solve
        # leaves an O(1) residual), not to measure any one type's accuracy.
        per_type_tol = 1e-4
        for solver_type in types:
            # both apply overloads for this type: by name, and by its default options Configuration
            by_name = make_vector(vec_cls, [0.0] * n)
            solver.apply(make_vector(vec_cls, rhs), by_name, solver_type)
            assert_solves(
                a,
                rhs,
                as_numpy(by_name, n),
                f"apply(type={solver_type!r})",
                per_type_tol,
            )

            by_options = make_vector(vec_cls, [0.0] * n)
            solver.apply(
                make_vector(vec_cls, rhs), by_options, solver_cls.options(solver_type)
            )
            assert_solves(
                a,
                rhs,
                as_numpy(by_options, n),
                f"apply(options={solver_type!r})",
                per_type_tol,
            )

    @settings(deadline=None)
    @given(system=spd_system())
    def test_direct_solution_matches_numpy(self, cls, system):
        # The issue's stated acceptance property, x ~ numpy.linalg.solve(A, b), as a solution-vector
        # comparison. A direct solve is well-determined to ~machine precision, so its solution can be
        # compared to numpy at a tight tolerance without coupling to an iterative type's stopping
        # criterion (that coupling, amplified by cond(A), is exactly why test_every_solver_type_solves
        # checks the residual instead). We therefore pin an *explicit* direct type rather than the
        # default, which is direct only for the dense backends -- the sparse backends default to an
        # iterative type (bicgstab.ssor for ISTL, bicgstab.ilut for Eigen). If a backend exposes no
        # direct type at all (e.g. ISTL built without SuperLU/UMFPACK), the comparison does not apply
        # and is skipped rather than run against an iterative solve.
        a, rhs = system
        n = len(rhs)
        vec_cls = matching_vector_class(cls)
        solver_cls = solver_for(cls)
        direct = direct_solver_types(solver_cls)
        if not direct:
            pytest.skip(
                "backend exposes no direct solver type to compare against numpy"
            )
        solver = solver_cls(make_matrix(cls, a))
        solution = make_vector(vec_cls, [0.0] * n)
        solver.apply(make_vector(vec_cls, rhs), solution, direct[0])

        expected = np.linalg.solve(a, rhs)
        scale = float(np.linalg.norm(expected)) + 1.0
        assert as_numpy(solution, n) == pytest.approx(
            expected, rel=1e-6, abs=1e-8 * scale
        )

    @settings(deadline=None)
    @given(system=spd_system())
    def test_every_type_solves_with_checks_disabled(self, cls, system):
        # Same per-type sweep as test_every_solver_type_solves, but with the internal post-solve
        # checks turned off (see options_with_checks_disabled): this reaches the "check disabled"
        # branches of solver/{common,eigen,istl}.hh that neither the default apply nor solver.tpl
        # exercise, since those checks default to enabled. The solve itself is unchanged, so a
        # correct backend still returns a solving x -- verified from the outside by residual here.
        a, rhs = system
        n = len(rhs)
        vec_cls = matching_vector_class(cls)
        solver_cls = solver_for(cls)
        solver = solver_cls(make_matrix(cls, a))

        types = solver_cls.types()
        assert types, "Solver advertises no types"
        for solver_type in types:
            solution = make_vector(vec_cls, [0.0] * n)
            solver.apply(
                make_vector(vec_cls, rhs),
                solution,
                options_with_checks_disabled(solver_cls, solver_type),
            )
            assert_solves(
                a,
                rhs,
                as_numpy(solution, n),
                f"apply(checks-off, {solver_type!r})",
                1e-4,
            )

    @settings(deadline=None)
    @given(system=spd_system())
    def test_solver_reuse_across_right_hand_sides(self, cls, system):
        # One Solver instance, several different right-hand sides. This guards the matrix-reference
        # lifetime documented in this module's docstring (the keep_alive fix) past the single
        # apply() the other tests make: if matrix_ dangled, the second or later apply() -- long after
        # the constructor returned -- would read freed memory. It also confirms the backends that
        # cache a factorization keep solving correctly when only the rhs changes.
        a, rhs = system
        n = len(rhs)
        vec_cls = matching_vector_class(cls)
        solver = solver_for(cls)(make_matrix(cls, a))

        # deterministic, genuinely distinct rhs directions derived from the drawn one
        right_hand_sides = [rhs, -rhs, 2.0 * rhs, rhs[::-1] + 1.0]
        for index, b in enumerate(right_hand_sides):
            solution = make_vector(vec_cls, [0.0] * n)
            solver.apply(make_vector(vec_cls, b), solution)
            assert_solves(a, b, as_numpy(solution, n), f"reuse #{index}")


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
