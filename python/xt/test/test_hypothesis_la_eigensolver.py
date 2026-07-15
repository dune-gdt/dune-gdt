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
    """Construct solver_cls(mat) with compute_eigenvectors explicitly disabled.

    EigenSolverOptions defaults *both* compute_eigenvalues and compute_eigenvectors to true (see
    dune/xt/la/eigen-solver/internal/base.hh's default_eigen_solver_options()), so the plain
    solver_cls(mat) constructor silently also computes eigenvectors. This property test only
    checks eigenvalues, so requesting eigenvectors is both wasted work and -- for the LAPACK-backed
    default.hh specialization CommonDenseMatrix/CommonSparseMatrixCsr use -- currently crashes (a
    newly-discovered defect in the eigenvector computation path's thread_local workspace caching in
    dune/xt/la/eigen-solver/internal/lapacke.hh, only reachable now that WP5 binds this at all; see
    the PR description). Explicitly disabling it here keeps this test scoped to what it actually
    verifies and avoids the crash.
    """
    opts = dict(solver_cls.options())
    opts["compute_eigenvectors"] = "false"
    return solver_cls(mat, opts)


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


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
