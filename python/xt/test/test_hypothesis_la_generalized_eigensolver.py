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
"""Property test for the bound LA::GeneralizedEigenSolver (coverage work package #342, part of
the #341 Codecov-driven effort): generalized eigenvalues of random real symmetric
positive-definite (SPD) matrix pairs must match scipy.linalg.eigh, taking the 0%-covered
generalized-eigen-solver headers (internal/base.hh, default.hh, generalized-eigen-solver.hh)
off zero by driving the LAPACKE dsygv code path from Python.

Each `<Matrix>GeneralizedEigenSolver` dune.xt.la binding (see python/xt/dune/xt/la/bindings.cc)
is exercised for every matrix class
dune.xt.test.hypothesis_strategies.discover_generalized_eigen_solver_types() finds -- so this
automatically covers whichever combination a given build actually bound, and the test self-skips
when LAPACKE is not linked (GeneralizedEigenSolverOptions::types() throws in that case, caught
by _has_lapack_backend() at module load time).

The generalized eigensolver has no pure-C++ fallback: the only available type is "lapack"
(dune/xt/la/generalized-eigen-solver/default.hh), which calls LAPACKE_dsygv through
Dune::XT::Common::Lapacke::dsygv (dune/xt/common/lapacke.cc). Linking LAPACKE (via the
OpenBLAS fallback added to cmake/modules/FindLAPACKE.cmake under #349) therefore unlocks both
this test and the C++ coverage it measures.

Beyond the eigenvalue accessors, the assertion-option tests below drive the
GeneralizedEigenSolverBase::post_checks() branches in internal/base.hh -- the largest 0% block
#342 targets -- that the accessors alone never reach: "assert_positive_eigenvalues" and
"assert_real_eigenvalues" are both satisfiable for SPD pairs (real, strictly positive
eigenvalues), so the branch bodies (and the compute_real_eigenvalues() assert-tolerance path
they trigger) run without tripping.

TestGeneralizedEigenSolverBindings at the end of this module complements the property tests with
deterministic checks of the remainder of bind_GeneralizedEigenSolver()'s surface (the free
factory overloads, the matrix/option properties, the eigenvector accessors) and of the failure
paths -- rejected options, mismatched sizes, a right hand side lapack cannot factorize.
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    dense_sparsity_pattern,
    discover_generalized_eigen_solver_types,
)

scipy_linalg = pytest.importorskip("scipy.linalg")


def _has_lapack_backend(cls):
    """Return True iff cls's GeneralizedEigenSolver reports a usable 'lapack' type.

    GeneralizedEigenSolverOptions::types() (dune/xt/la/generalized-eigen-solver/default.hh)
    throws Dune::Exception (surfaced as DuneError in Python) when LAPACKE is not linked (the
    types list would be empty). We catch precisely that here so the module-level
    MATRIX_CLASSES list stays empty and the test class is skipped cleanly instead of failing
    at import time; any other exception propagates so a real bug is not silently hidden.
    """
    import dune.xt.la as la
    from dune.xt.common import DuneError

    solver_cls = getattr(la, cls.__name__ + "GeneralizedEigenSolver", None)
    if solver_cls is None:
        return False
    try:
        return "lapack" in solver_cls.types()
    except DuneError:
        return False


MATRIX_CLASSES = [
    cls for cls in discover_generalized_eigen_solver_types() if _has_lapack_backend(cls)
]


@st.composite
def spd_matrix_pair(draw, min_size=1, max_size=6, bound=5.0):
    """A pair of independent random real SPD matrices of the same size.

    A = M^T M + n*I is symmetric and positive-definite for any M: M^T M is symmetric PSD
    and the n*I shift raises every eigenvalue to at least n >= 1. Independent fill matrices
    for lhs and rhs produce a non-trivial generalized problem lhs*v = lambda*rhs*v (if both
    were the same matrix the problem would collapse to lambda=1 for all v).
    """
    n = draw(st.integers(min_size, max_size))
    entries_lhs = draw(
        st.lists(
            st.floats(-bound, bound, allow_nan=False, allow_infinity=False),
            min_size=n * n,
            max_size=n * n,
        )
    )
    entries_rhs = draw(
        st.lists(
            st.floats(-bound, bound, allow_nan=False, allow_infinity=False),
            min_size=n * n,
            max_size=n * n,
        )
    )
    m_lhs = np.asarray(entries_lhs).reshape(n, n)
    m_rhs = np.asarray(entries_rhs).reshape(n, n)
    lhs = m_lhs.T @ m_lhs + n * np.eye(n)
    rhs = m_rhs.T @ m_rhs + n * np.eye(n)
    return lhs, rhs


def make_matrix(cls, arr):
    rows, cols = arr.shape
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    for ii in range(rows):
        for jj in range(cols):
            mat.set_entry(ii, jj, float(arr[ii, jj]))
    return mat


def generalized_solver_for(cls):
    import dune.xt.la as la

    return getattr(la, cls.__name__ + "GeneralizedEigenSolver")


def solver_with_asserts(cls, lhs_arr, rhs_arr, **assert_opts):
    """Build a solver on the "lapack" defaults, overriding the given (string-valued) option keys.

    Mirrors the option-tweaking pattern of the sibling eigensolver test: start from the bound
    static options() for the pinned type (the only type here is "lapack") and overwrite specific
    keys, so post_checks() branches gated on assert_* tolerances can be switched on from Python.
    """
    solver_cls = generalized_solver_for(cls)
    opts = dict(solver_cls.options("lapack"))
    opts.update(assert_opts)
    return solver_cls(make_matrix(cls, lhs_arr), make_matrix(cls, rhs_arr), opts)


@pytest.mark.skipif(
    not MATRIX_CLASSES,
    reason="no generalized eigen-solver with lapack backend available",
)
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestGeneralizedEigenSolverAgainstScipy:
    @settings(deadline=None)
    @given(pair=spd_matrix_pair())
    def test_eigenvalues_match_scipy(self, cls, pair):
        lhs_arr, rhs_arr = pair
        solver = generalized_solver_for(cls)(
            make_matrix(cls, lhs_arr), make_matrix(cls, rhs_arr)
        )

        actual = np.sort(np.array([ev.real for ev in solver.eigenvalues()]))
        actual_imag = np.array([ev.imag for ev in solver.eigenvalues()])
        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))

        # SPD pair -> real positive eigenvalues (up to solver round-off)
        assert actual_imag == pytest.approx(np.zeros_like(actual_imag), abs=1e-8)
        assert actual == pytest.approx(expected, rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(pair=spd_matrix_pair())
    def test_real_eigenvalues_match_scipy(self, cls, pair):
        # real_eigenvalues() is a distinct accessor from eigenvalues() (base.hh): it returns
        # the real-typed spectrum, valid here precisely because SPD pairs have real eigenvalues.
        lhs_arr, rhs_arr = pair
        solver = generalized_solver_for(cls)(
            make_matrix(cls, lhs_arr), make_matrix(cls, rhs_arr)
        )

        actual = np.sort(np.array(solver.real_eigenvalues()))
        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))
        assert actual == pytest.approx(expected, rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(pair=spd_matrix_pair())
    def test_min_and_max_eigenvalue_match_scipy(self, cls, pair):
        lhs_arr, rhs_arr = pair
        solver = generalized_solver_for(cls)(
            make_matrix(cls, lhs_arr), make_matrix(cls, rhs_arr)
        )

        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))
        assert solver.min_eigenvalues(1)[0] == pytest.approx(
            expected[0], rel=1e-6, abs=1e-8
        )
        assert solver.max_eigenvalues(1)[0] == pytest.approx(
            expected[-1], rel=1e-6, abs=1e-8
        )

    @settings(deadline=None)
    @given(pair=spd_matrix_pair(min_size=2))
    def test_min_and_max_eigenvalues_return_k_extremes(self, cls, pair):
        # min/max_eigenvalues(k) for k > 1 exercises the partial-sort branch in
        # GeneralizedEigenSolverBase (base.hh), which min/max_eigenvalues(1) above does not.
        lhs_arr, rhs_arr = pair
        n = lhs_arr.shape[0]
        k = min(2, n)
        solver = generalized_solver_for(cls)(
            make_matrix(cls, lhs_arr), make_matrix(cls, rhs_arr)
        )
        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))

        smallest = np.sort(np.array(solver.min_eigenvalues(k)))
        largest = np.sort(np.array(solver.max_eigenvalues(k)))
        assert smallest == pytest.approx(expected[:k], rel=1e-6, abs=1e-8)
        assert largest == pytest.approx(expected[-k:], rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(pair=spd_matrix_pair())
    def test_assert_positive_eigenvalues_option(self, cls, pair):
        # A generalized eigenvalue lhs*v = lambda*rhs*v with SPD lhs and SPD rhs is bounded below
        # by lambda_min(lhs)/lambda_max(rhs) > 0, so every eigenvalue is strictly positive. Setting
        # "assert_positive_eigenvalues" to a small positive tolerance therefore drives the
        # positive-eigenvalue post-check branch in GeneralizedEigenSolverBase::post_checks()
        # (base.hh) -- and the compute_real_eigenvalues() path it triggers -- without tripping the
        # assertion. The tolerance is far below the smallest reachable eigenvalue (>~ 1/151 for the
        # generated bound/size), so the check always passes.
        lhs_arr, rhs_arr = pair
        solver = solver_with_asserts(
            cls, lhs_arr, rhs_arr, assert_positive_eigenvalues="1e-8"
        )
        actual = np.sort(np.array(solver.real_eigenvalues()))
        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))
        assert actual[0] > 0
        assert actual == pytest.approx(expected, rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(pair=spd_matrix_pair())
    def test_assert_real_eigenvalues_option(self, cls, pair):
        # "assert_real_eigenvalues" > 0 drives the imaginary-part tolerance check inside
        # compute_real_eigenvalues() (base.hh). dsygv fills the imaginary part with an exact 0
        # (internal/lapacke.hh), so the check passes for every SPD pair regardless of tolerance.
        lhs_arr, rhs_arr = pair
        solver = solver_with_asserts(
            cls, lhs_arr, rhs_arr, assert_real_eigenvalues="1e-8"
        )
        actual = np.sort(np.array([ev.real for ev in solver.eigenvalues()]))
        expected = np.sort(scipy_linalg.eigh(lhs_arr, rhs_arr, eigvals_only=True))
        assert actual == pytest.approx(expected, rel=1e-6, abs=1e-8)


@pytest.mark.skipif(
    not MATRIX_CLASSES,
    reason="no generalized eigen-solver with lapack backend available",
)
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestGeneralizedEigenSolverBindings:
    """Deterministic coverage of the parts of bind_GeneralizedEigenSolver() the property tests miss.

    The class above drives the eigenvalue accessors only; everything bound alongside them in
    python/xt/dune/xt/la/generalized_eigen_solver.hh -- the two free `make_generalized_eigen_solver`
    factory overloads, the `used_options`/`lhs_matrix`/`rhs_matrix` properties and the
    `eigenvectors()`/`real_eigenvectors()` accessors -- has no test at all. These are fixed small
    problems rather than hypothesis-generated ones: none of them is about numerics.

    The pair used throughout is lhs = [3 1; 1 3], rhs = [2 0; 0 2], whose generalized eigenvalues are
    exactly 1 and 2 (with rhs = 2*Id the problem reduces to the eigenvalues of lhs/2, and [a b; b a]
    has the eigenvalues a +- b).
    """

    LHS = np.array([[3.0, 1.0], [1.0, 3.0]])
    RHS = np.array([[2.0, 0.0], [0.0, 2.0]])
    EIGENVALUES = [1.0, 2.0]

    def test_types_and_options(self, cls):
        solver_cls = generalized_solver_for(cls)
        types = list(solver_cls.types())
        assert "lapack" in types
        # options() without an argument has to default to the first available type
        assert dict(solver_cls.options())["type"] == types[0]
        for tp in types:
            assert dict(solver_cls.options(tp))["type"] == tp

    def test_matrices_and_used_options_are_exposed(self, cls):
        solver = generalized_solver_for(cls)(
            make_matrix(cls, self.LHS), make_matrix(cls, self.RHS)
        )

        assert dict(solver.used_options)["type"] == "lapack"
        for prop, expected in (
            (solver.lhs_matrix, self.LHS),
            (solver.rhs_matrix, self.RHS),
        ):
            assert prop.rows == expected.shape[0]
            assert prop.cols == expected.shape[1]
            for ii in range(expected.shape[0]):
                for jj in range(expected.shape[1]):
                    assert prop.get_entry(ii, jj) == pytest.approx(expected[ii, jj])

    def test_factory_matches_constructor(self, cls):
        import dune.xt.la as la

        opts = dict(generalized_solver_for(cls).options("lapack"))
        solvers = [
            generalized_solver_for(cls)(
                make_matrix(cls, self.LHS), make_matrix(cls, self.RHS)
            ),
            la.make_generalized_eigen_solver(
                make_matrix(cls, self.LHS), make_matrix(cls, self.RHS), "lapack"
            ),
            la.make_generalized_eigen_solver(
                make_matrix(cls, self.LHS), make_matrix(cls, self.RHS), opts
            ),
        ]
        for solver in solvers:
            actual = np.sort(np.array([ev.real for ev in solver.eigenvalues()]))
            assert actual == pytest.approx(self.EIGENVALUES)

    def test_eigenvectors_are_not_available(self, cls):
        # Eigenvectors of a generalized problem are not implemented (see
        # dune/xt/la/generalized-eigen-solver/default.hh): requesting them up front makes compute()
        # fail, and asking for them afterwards is reported as a usage error rather than silently
        # returning an empty matrix.
        from dune.xt.common import DuneError

        solver_cls = generalized_solver_for(cls)
        solver = solver_cls(make_matrix(cls, self.LHS), make_matrix(cls, self.RHS))
        with pytest.raises(DuneError):
            solver.eigenvectors()
        with pytest.raises(DuneError):
            solver.real_eigenvectors()

        opts = dict(solver_cls.options("lapack"))
        opts["compute_eigenvectors"] = "true"
        eigenvector_solver = solver_cls(
            make_matrix(cls, self.LHS), make_matrix(cls, self.RHS), opts
        )
        with pytest.raises(DuneError):
            eigenvector_solver.eigenvalues()

    def test_negative_definite_pair(self, cls):
        # The mirror image of test_assert_positive_eigenvalues_option above: a negative definite lhs
        # with a positive definite rhs has a strictly negative spectrum, which is the only way to
        # satisfy "assert_negative_eigenvalues" (and hence to reach the non-throwing side of that
        # post check in GeneralizedEigenSolverBase::post_checks()).
        solver = solver_with_asserts(
            cls, -self.LHS, self.RHS, assert_negative_eigenvalues="1e-10"
        )
        actual = np.sort(np.array(solver.real_eigenvalues()))
        assert actual == pytest.approx([-2.0, -1.0])

    def test_violated_sign_assertions_are_reported(self, cls):
        from dune.xt.common import DuneError

        # Constructed outside the raises block on purpose: the assertion is a post check, so
        # construction has to succeed and only the eigenvalue access may fail.
        negative_on_positive_spectrum = solver_with_asserts(
            cls, self.LHS, self.RHS, assert_negative_eigenvalues="1e-10"
        )
        with pytest.raises(DuneError):
            negative_on_positive_spectrum.eigenvalues()

        positive_on_negative_spectrum = solver_with_asserts(
            cls, -self.LHS, self.RHS, assert_positive_eigenvalues="1e-10"
        )
        with pytest.raises(DuneError):
            positive_on_negative_spectrum.eigenvalues()

    def test_indefinite_rhs_is_reported(self, cls):
        # lapack's dsygv requires a positive definite rhs; a zero one passes all of our own checks
        # (square, of matching size, free of inf/nan), so the failure can only come from the backend.
        from dune.xt.common import DuneError

        solver = generalized_solver_for(cls)(
            make_matrix(cls, self.LHS), make_matrix(cls, np.zeros((2, 2)))
        )
        with pytest.raises(DuneError):
            solver.eigenvalues()

    @pytest.mark.parametrize(
        "broken_option",
        [
            {"type": "this_generalized_eigen_solver_type_does_not_exist"},
            {"real_tolerance": "0"},
            {
                "assert_positive_eigenvalues": "1e-10",
                "assert_negative_eigenvalues": "1e-10",
            },
        ],
        ids=["unknown-type", "non-positive-tolerance", "contradicting-assertions"],
    )
    def test_inconsistent_options_are_rejected(self, cls, broken_option):
        from dune.xt.common import DuneError

        with pytest.raises(DuneError):
            solver_with_asserts(cls, self.LHS, self.RHS, **broken_option)

    def test_matrices_of_wrong_size_are_rejected(self, cls):
        from dune.xt.common import DuneError

        square = make_matrix(cls, self.RHS)
        non_square = make_matrix(cls, np.ones((2, 3)))
        larger_square = make_matrix(cls, np.eye(3))
        solver_cls = generalized_solver_for(cls)
        for lhs, rhs in (
            (non_square, square),
            (square, larger_square),
            (square, non_square),
        ):
            with pytest.raises(DuneError):
                solver_cls(lhs, rhs)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
