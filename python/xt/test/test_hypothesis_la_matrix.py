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
"""Property-based tests of the real (double) LA matrix containers against a numpy oracle.

The complementary complex-field matrix suite already lives in
test_hypothesis_la_complex_container.py; the double-field vectors in test_hypothesis_la_container.py.
This closes the remaining gap -- the double-field *matrix* containers -- so every bound
(backend x field) matrix combination has a runtime property suite. Like the vector suite it walks
whatever dune.xt.la actually bound (discover_matrix_types), so dense/sparse Common, Istl and Eigen
are all covered without hard-coding the list, and hypothesis supplies the payloads.

The operations exercised here (matrix-vector product mv()/mtv(), scal(), axpy(), the +/- operators,
unit_row()/unit_col() and sup_norm()) are the per-backend overrides in
dune/xt/la/container/{istl,eigen,common}*.hh, which the fixed-input C++ .tpl tests only reach for a
handful of hand-written matrices.
"""

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    dense_sparsity_pattern,
    discover_matrix_types,
    discover_vector_types,
    finite_floats,
)

MATRIX_CLASSES = list(discover_matrix_types(field="double"))
VECTOR_CLASSES = list(discover_vector_types(field="double"))

# relative tolerance for comparisons against numpy: both sides accumulate in double, but not
# necessarily in the same order, so exact equality only holds for order-independent quantities
RTOL = 1e-12


def make_vector(cls, values):
    vec = cls(len(values), 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def make_matrix(cls, arr):
    rows, cols = arr.shape
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    for ii in range(rows):
        for jj in range(cols):
            mat.set_entry(ii, jj, float(arr[ii, jj]))
    return mat


def matrix_as_numpy(mat):
    return np.array(
        [[mat.get_entry(ii, jj) for jj in range(mat.cols)] for ii in range(mat.rows)]
    )


def matching_vector_class(matrix_cls):
    """The double vector class dune.xt.la pairs matrix_cls with, by shared backend prefix.

    Mirrors test_hypothesis_la_complex_container.matching_vector_class: every Common-family matrix
    (dense or sparse) is paired with the same CommonVector by addbind_Matrix_Vector_interaction in
    bindings.cc, Istl matrices with IstlVector, Eigen matrices with EigenVector.
    """
    name = matrix_cls.__name__
    for vector_cls in VECTOR_CLASSES:
        prefix = vector_cls.__name__[: -len("Vector")]
        if name.startswith(prefix):
            return vector_cls
    raise AssertionError(f"no double vector class found for {name}")


@st.composite
def matrix_arrays(draw, min_dim=1, max_dim=8, bound=1e3, square=False):
    rows = draw(st.integers(min_dim, max_dim))
    cols = rows if square else draw(st.integers(min_dim, max_dim))
    entries = draw(
        st.lists(finite_floats(bound), min_size=rows * cols, max_size=rows * cols)
    )
    return np.asarray(entries).reshape(rows, cols)


@pytest.mark.skipif(not MATRIX_CLASSES, reason="no double LA matrix binding available")
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestMatrixAgainstNumpy:
    @given(arr=matrix_arrays())
    def test_entry_roundtrip_and_shape(self, cls, arr):
        mat = make_matrix(cls, arr)
        assert mat.rows == arr.shape[0]
        assert mat.cols == arr.shape[1]
        assert matrix_as_numpy(mat) == pytest.approx(arr, abs=0.0)

    @given(arr=matrix_arrays(), data=st.data())
    def test_matvec_and_transpose_matvec(self, cls, arr, data):
        rows, cols = arr.shape
        mat = make_matrix(cls, arr)
        vec_cls = matching_vector_class(cls)

        xs = data.draw(
            st.lists(finite_floats(1e3), min_size=cols, max_size=cols), label="mv x"
        )
        yv = mat.dot(make_vector(vec_cls, xs))
        y_expected = arr @ np.asarray(xs)
        y_actual = np.array([yv.get_entry(ii) for ii in range(rows)])
        assert y_actual == pytest.approx(y_expected, rel=1e-9, abs=1e-6)

        # mtv computes A^T @ x, so its source has length rows and its range length cols
        zs = data.draw(
            st.lists(finite_floats(1e3), min_size=rows, max_size=rows), label="mtv x"
        )
        out = make_vector(vec_cls, [0.0] * cols)
        mat.mtv(make_vector(vec_cls, zs), out)
        out_expected = arr.T @ np.asarray(zs)
        out_actual = np.array([out.get_entry(jj) for jj in range(cols)])
        assert out_actual == pytest.approx(out_expected, rel=1e-9, abs=1e-6)

    @given(arr=matrix_arrays(), alpha=finite_floats(bound=1e3))
    def test_scal(self, cls, arr, alpha):
        mat = make_matrix(cls, arr)
        mat.scal(alpha)
        assert matrix_as_numpy(mat) == pytest.approx(alpha * arr, rel=RTOL, abs=1e-300)

    @given(
        shape=st.tuples(st.integers(1, 8), st.integers(1, 8)),
        data=st.data(),
        alpha=finite_floats(bound=1e3),
    )
    def test_axpy_leaves_argument_untouched(self, cls, shape, data, alpha):
        rows, cols = shape
        a = np.asarray(
            data.draw(
                st.lists(finite_floats(1e3), min_size=rows * cols, max_size=rows * cols)
            )
        ).reshape(rows, cols)
        b = np.asarray(
            data.draw(
                st.lists(finite_floats(1e3), min_size=rows * cols, max_size=rows * cols)
            )
        ).reshape(rows, cols)
        xm, ym = make_matrix(cls, a), make_matrix(cls, b)
        xm.axpy(alpha, ym)
        assert matrix_as_numpy(xm) == pytest.approx(a + alpha * b, rel=RTOL, abs=1e-300)
        # axpy must not touch its argument
        assert matrix_as_numpy(ym) == pytest.approx(b, abs=0.0)

    @given(arr=matrix_arrays())
    def test_copy_is_deep(self, cls, arr):
        mat = make_matrix(cls, arr)
        clone = mat.copy(True)
        clone.set_entry(0, 0, clone.get_entry(0, 0) + 1.0)
        assert mat.get_entry(0, 0) == arr[0, 0]

    @given(arr=matrix_arrays())
    def test_sup_norm_is_max_abs_entry(self, cls, arr):
        # MatrixInterface::sup_norm() (dune/xt/la/container/matrix-interface.hh) is the largest
        # magnitude entry -- an order-independent max, so this holds exactly.
        mat = make_matrix(cls, arr)
        assert mat.sup_norm() == pytest.approx(float(np.abs(arr).max()), rel=RTOL)

    @given(
        shape=st.tuples(st.integers(1, 8), st.integers(1, 8)),
        data=st.data(),
    )
    def test_neg_and_add_sub(self, cls, shape, data):
        rows, cols = shape
        a = np.asarray(
            data.draw(
                st.lists(finite_floats(1e3), min_size=rows * cols, max_size=rows * cols)
            )
        ).reshape(rows, cols)
        b = np.asarray(
            data.draw(
                st.lists(finite_floats(1e3), min_size=rows * cols, max_size=rows * cols)
            )
        ).reshape(rows, cols)
        am, bm = make_matrix(cls, a), make_matrix(cls, b)
        assert matrix_as_numpy(am.neg()) == pytest.approx(-a, abs=0.0)
        # neg is out-of-place
        assert matrix_as_numpy(am) == pytest.approx(a, abs=0.0)
        assert matrix_as_numpy(am + bm) == pytest.approx(a + b, rel=RTOL, abs=1e-300)
        assert matrix_as_numpy(am - bm) == pytest.approx(a - b, rel=RTOL, abs=1e-300)

    @given(arr=matrix_arrays(square=True))
    def test_unit_row_and_unit_col(self, cls, arr):
        n = arr.shape[0]
        row_mat = make_matrix(cls, arr)
        row_mat.unit_row(0)
        expected = arr.copy()
        expected[0, :] = 0.0
        expected[0, 0] = 1.0
        assert matrix_as_numpy(row_mat) == pytest.approx(expected, abs=0.0)

        col_mat = make_matrix(cls, arr)
        col_mat.unit_col(n - 1)
        expected = arr.copy()
        expected[:, n - 1] = 0.0
        expected[n - 1, n - 1] = 1.0
        assert matrix_as_numpy(col_mat) == pytest.approx(expected, abs=0.0)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
