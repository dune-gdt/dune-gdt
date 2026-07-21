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
"""Property test for the bound LA::GeneralizedEigenSolver (WP5, #349): generalized eigenvalues
of random real symmetric positive-definite (SPD) matrix pairs must match scipy.linalg.eigh,
exercising the LAPACKE dsygv code path that the cmake fix in issue #349 makes reachable in
the coverage build.

Each `<Matrix>GeneralizedEigenSolver` dune.xt.la binding (see python/xt/dune/xt/la/bindings.cc)
is exercised for every matrix class
dune.xt.test.hypothesis_strategies.discover_generalized_eigen_solver_types() finds -- so this
automatically covers whichever combination a given build actually bound, and the test self-skips
when LAPACKE is not linked (GeneralizedEigenSolverOptions::types() throws in that case, caught
by _has_lapack_backend() at module load time).

The generalized eigensolver has no pure-C++ fallback: the only available type is "lapack"
(dune/xt/la/generalized-eigen-solver/default.hh), which calls LAPACKE_dsygv through
Dune::XT::Common::Lapacke::dsygv (dune/xt/common/lapacke.cc). Linking LAPACKE (via the
OpeBLAS fallback added to cmake/modules/FindLAPACKE.cmake) therefore unlocks both this test
and the C++ coverage it measures.
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
    throws Dune::Exception when LAPACKE is not linked (the types list would be empty). We
    catch that here so the module-level MATRIX_CLASSES list stays empty and the test class
    is skipped cleanly instead of failing at import time.
    """
    import dune.xt.la as la

    solver_cls = getattr(la, cls.__name__ + "GeneralizedEigenSolver", None)
    if solver_cls is None:
        return False
    try:
        return "lapack" in solver_cls.types()
    except Exception:
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


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
