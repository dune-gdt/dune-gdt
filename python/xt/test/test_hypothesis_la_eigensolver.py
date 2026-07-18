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
"""Property test for the bound LA::EigenSolver (WP5, #320): eigenvalues of random real symmetric
positive-definite (SPD) matrices must match scipy.linalg.eigvalsh, the acceptance criterion #320
states explicitly for this work package.

Each `<Matrix>EigenSolver` dune.xt.la binds (see python/xt/dune/xt/la/bindings.cc) is exercised
for every matrix class dune.xt.test.hypothesis_strategies.discover_eigen_solver_types() finds --
so this automatically covers whichever combination a given build actually bound.
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    dense_sparsity_pattern,
    discover_eigen_solver_types,
)

scipy_linalg = pytest.importorskip("scipy.linalg")

MATRIX_CLASSES = list(discover_eigen_solver_types())


@st.composite
def spd_matrices(draw, min_size=1, max_size=6, bound=10.0):
    """A random real symmetric positive-definite matrix: A^T A + n * I is always SPD."""
    n = draw(st.integers(min_size, max_size))
    entries = draw(
        st.lists(
            st.floats(-bound, bound, allow_nan=False, allow_infinity=False),
            min_size=n * n,
            max_size=n * n,
        )
    )
    a = np.asarray(entries).reshape(n, n)
    return a.T @ a + n * np.eye(n)


def make_matrix(cls, arr):
    rows, cols = arr.shape
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    for ii in range(rows):
        for jj in range(cols):
            mat.set_entry(ii, jj, float(arr[ii, jj]))
    return mat


def eigen_solver_for(cls):
    import dune.xt.la as la

    return getattr(la, cls.__name__ + "EigenSolver")


def make_eigenvalues_only_solver(solver_cls, mat):
    """Construct solver_cls(mat) with type="shifted_qr", eigenvector computation disabled, and the
    eigendecomposition post-check disabled to match.

    EigenSolverOptions defaults *both* compute_eigenvalues and compute_eigenvectors to true (see
    dune/xt/la/eigen-solver/internal/base.hh's default_eigen_solver_options()), so the plain
    solver_cls(mat) constructor silently also computes eigenvectors -- wasted work for this
    eigenvalues-only property test, so we disable it explicitly. default_eigen_solver_options()
    *also* defaults "assert_eigendecomposition" to a positive tolerance ("1e-10", the only assert_*
    key that defaults enabled), which would otherwise make EigenSolverBase::post_checks() (run
    after every compute()) unconditionally check the eigendecomposition -- pointless work here, and
    outright unsafe for the LAPACK-backed "lapack" type specifically, where eigenvectors_ stays null
    when compute_eigenvectors=false (dune/xt/la/eigen-solver/internal/base.hh:691-705 dereferences
    it regardless). We disable it explicitly to avoid depending on that.

    We also pin the solver type to "shifted_qr" rather than leaving it at the default (which picks
    "lapack" whenever LAPACK is available): a separate, still-open defect in the LAPACK-backed
    eigenvalues-*and*-eigenvectors branch (see the PR description) makes "lapack" risky here even
    though this test itself only asks for eigenvalues. "shifted_qr" is a pure-C++ fallback,
    extensively covered by the existing C++ test suite
    (dune/xt/test/la/eigensolver_for_real_matrix_with_real_evs_from_*.tpl, down to 1e-14
    tolerances) -- and, since spd_matrices() below generates 1x1 matrices too (min_size=1),
    exercising it here is also what surfaced a genuine, previously-latent bug in
    dune/xt/la/eigen-solver/internal/shifted-qr.hh's hessenberg_transformation(): its loop bound
    `rows(A) - 2` is unsigned and underflows for any matrix with fewer than 2 rows, turning the
    "nothing to do for a matrix this small" case into a walk off the end of the matrix. Fixed
    directly in shifted-qr.hh (rewritten as `jj + 2 < rows(A)`, which is well-defined and correctly
    empty for rows(A) < 2) rather than avoided here, since real callers can construct an
    EigenSolver for a 1x1 (or 0x0) matrix just as easily as this property test does.
    """
    opts = dict(solver_cls.options("shifted_qr"))
    opts["compute_eigenvectors"] = "false"
    opts["assert_eigendecomposition"] = "-1"
    return solver_cls(mat, opts)


def make_full_solver(solver_cls, mat):
    """Construct solver_cls(mat) pinned to "shifted_qr" with eigenvector computation left on and the
    eigendecomposition post-check enabled.

    This is the counterpart of make_eigenvalues_only_solver: where that one disables everything but
    the eigenvalues (to keep the eigenvalue-vs-scipy property cheap), this one keeps
    compute_eigenvectors at its default_eigen_solver_options() value (on) so the eigenvector
    accessors return populated data and EigenSolverBase::post_checks() actually runs the
    T*lambda*T^-1 == A check internally -- the branches in dune/xt/la/eigen-solver/internal/base.hh
    (complex_eigendecomposition_check / assert_eigendecomposition) that the eigenvalues-only path
    skips.

    assert_eigendecomposition is an *absolute* tolerance (base.hh compares |error_ij| > tolerance),
    so its 1e-10 default would spuriously trip on the larger-magnitude SPD matrices spd_matrices()
    generates (entries can reach the hundreds). We pin it to 1e-6, still positive -- so the
    post-check branch is exercised -- but comfortably above the ~1e-9 worst-case reconstruction
    error for these small, bounded, +n*I-regularized systems. "shifted_qr" (pure C++, exercised by
    the existing eigensolver_for_real_matrix_with_real_evs*.tpl suite) is used rather than the
    default "lapack" for the same reason documented in make_eigenvalues_only_solver: the LAPACK
    eigenvector branch has a still-open defect.
    """
    opts = dict(solver_cls.options("shifted_qr"))
    opts["assert_eigendecomposition"] = "1e-6"
    return solver_cls(mat, opts)


def matrix_as_numpy(mat):
    return np.array(
        [[mat.get_entry(ii, jj) for jj in range(mat.cols)] for ii in range(mat.rows)],
        dtype=complex,
    )


@pytest.mark.skipif(not MATRIX_CLASSES, reason="no eigen-solver binding available")
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestEigenSolverAgainstScipy:
    @settings(deadline=None)
    @given(arr=spd_matrices())
    def test_eigenvalues_match_scipy(self, cls, arr):
        mat = make_matrix(cls, arr)
        solver = make_eigenvalues_only_solver(eigen_solver_for(cls), mat)

        actual = np.sort(np.array([ev.real for ev in solver.eigenvalues()]))
        actual_imag = np.array([ev.imag for ev in solver.eigenvalues()])
        expected = np.sort(scipy_linalg.eigvalsh(arr))

        # SPD input -> real eigenvalues (up to solver round-off)
        assert actual_imag == pytest.approx(np.zeros_like(actual_imag), abs=1e-8)
        assert actual == pytest.approx(expected, rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(arr=spd_matrices())
    def test_min_and_max_eigenvalue_match_scipy(self, cls, arr):
        mat = make_matrix(cls, arr)
        solver = make_eigenvalues_only_solver(eigen_solver_for(cls), mat)

        expected = np.sort(scipy_linalg.eigvalsh(arr))
        assert solver.min_eigenvalues(1)[0] == pytest.approx(
            expected[0], rel=1e-6, abs=1e-8
        )
        assert solver.max_eigenvalues(1)[0] == pytest.approx(
            expected[-1], rel=1e-6, abs=1e-8
        )

    @settings(deadline=None)
    @given(arr=spd_matrices(min_size=2))
    def test_min_and_max_eigenvalues_return_k_extremes(self, cls, arr):
        # min/max_eigenvalues(k) for k > 1 exercises the partial-sort branch in
        # EigenSolverBase (base.hh), which min/max_eigenvalues(1) above does not.
        n = arr.shape[0]
        k = min(2, n)
        solver = make_eigenvalues_only_solver(
            eigen_solver_for(cls), make_matrix(cls, arr)
        )
        expected = np.sort(scipy_linalg.eigvalsh(arr))

        smallest = np.sort(np.array(solver.min_eigenvalues(k)))
        largest = np.sort(np.array(solver.max_eigenvalues(k)))
        assert smallest == pytest.approx(expected[:k], rel=1e-6, abs=1e-8)
        assert largest == pytest.approx(expected[-k:], rel=1e-6, abs=1e-8)

    @settings(deadline=None)
    @given(arr=spd_matrices())
    def test_eigenvectors_satisfy_the_eigenrelation(self, cls, arr):
        # Enabling eigenvector computation + the eigendecomposition post-check drives the branches
        # in base.hh that the eigenvalues-only property test skips. A*V == V*diag(lambda) is
        # checked directly (order-consistent: eigenvalues() and eigenvectors() share a column
        # order), which sidesteps the non-uniqueness of eigenvectors for repeated eigenvalues.
        solver = make_full_solver(eigen_solver_for(cls), make_matrix(cls, arr))
        evals = np.array([complex(ev) for ev in solver.eigenvalues()])
        vectors = matrix_as_numpy(solver.eigenvectors())
        a = arr.astype(complex)
        residual = a @ vectors - vectors @ np.diag(evals)
        scale = float(np.linalg.norm(a)) + 1.0
        assert float(np.linalg.norm(residual)) == pytest.approx(0.0, abs=1e-6 * scale)

    @settings(deadline=None)
    @given(arr=spd_matrices())
    def test_eigenvectors_inverse_is_the_inverse(self, cls, arr):
        solver = make_full_solver(eigen_solver_for(cls), make_matrix(cls, arr))
        vectors = matrix_as_numpy(solver.eigenvectors())
        inverse = matrix_as_numpy(solver.eigenvectors_inverse())
        identity = np.eye(arr.shape[0], dtype=complex)
        assert vectors @ inverse == pytest.approx(identity, rel=1e-6, abs=1e-6)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
