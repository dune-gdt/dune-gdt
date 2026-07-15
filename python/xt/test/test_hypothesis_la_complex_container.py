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
"""std::complex<double>-valued LA container property tests (WP5, #320).

Mirrors test_hypothesis_la_container.py's structure, but for the complex-field vector and matrix
containers WP5 adds bindings for (see python/xt/dune/xt/la/container.bindings.hh and bindings.cc):
one dense backend per family (Common, Eigen, Istl) plus the sparse Common containers.
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

VECTOR_CLASSES = list(discover_vector_types(field="complex"))
MATRIX_CLASSES = list(discover_matrix_types(field="complex"))

# relative tolerance for comparisons against numpy: both sides accumulate in double, but not
# necessarily in the same order, so exact equality only holds for order-independent quantities
RTOL = 1e-12


def complex_floats(bound=1e6):
    return st.builds(complex, finite_floats(bound), finite_floats(bound))


@st.composite
def complex_vector_data(draw, min_size=1, max_size=64, bound=1e6):
    """Plain python lists of bounded finite complex numbers, the payload for complex LA tests."""
    return draw(st.lists(complex_floats(bound), min_size=min_size, max_size=max_size))


def make_vector(cls, values):
    vec = cls(len(values), 0j)
    for ii, value in enumerate(values):
        vec.set_entry(ii, complex(value))
    return vec


def as_numpy(vec):
    return np.array([vec.get_entry(ii) for ii in range(len(vec))], dtype=complex)


@pytest.mark.skipif(
    not VECTOR_CLASSES, reason="no complex-valued LA vector binding available"
)
@pytest.mark.parametrize("cls", VECTOR_CLASSES, ids=lambda c: c.__name__)
class TestComplexVectorAgainstNumpy:
    @given(data=complex_vector_data())
    def test_entry_roundtrip(self, cls, data):
        vec = make_vector(cls, data)
        assert vec.size == len(data)
        assert len(vec) == len(data)
        assert as_numpy(vec) == pytest.approx(np.asarray(data, dtype=complex), abs=0.0)

    @given(data=complex_vector_data())
    def test_norms(self, cls, data):
        vec = make_vector(cls, data)
        arr = np.asarray(data, dtype=complex)
        assert vec.l1_norm() == pytest.approx(np.sum(np.abs(arr)), rel=RTOL)
        assert vec.l2_norm() == pytest.approx(np.linalg.norm(arr, 2), rel=RTOL)
        assert vec.sup_norm() == pytest.approx(
            np.max(np.abs(arr)) if len(arr) else 0.0, rel=RTOL
        )

    @given(
        pair=st.integers(1, 64).flatmap(
            lambda n: st.tuples(complex_vector_data(n, n), complex_vector_data(n, n))
        )
    )
    def test_dot_is_conjugate_linear_in_the_first_argument(self, cls, pair):
        # VectorInterface::dot conjugates *this* (see complex_switch::dot in
        # dune/xt/la/container/vector-interface.hh), matching numpy's vdot (not dot) convention.
        xs, ys = pair
        xv, yv = make_vector(cls, xs), make_vector(cls, ys)
        expected = complex(
            np.vdot(np.asarray(xs, dtype=complex), np.asarray(ys, dtype=complex))
        )
        abs_tol = 1e-9 * max(
            1.0, float(np.sum(np.abs(np.asarray(xs)) * np.abs(np.asarray(ys))))
        )
        actual = xv.dot(yv)
        assert actual.real == pytest.approx(expected.real, rel=RTOL, abs=abs_tol)
        assert actual.imag == pytest.approx(expected.imag, rel=RTOL, abs=abs_tol)

    @given(
        pair=st.integers(1, 64).flatmap(
            lambda n: st.tuples(complex_vector_data(n, n), complex_vector_data(n, n))
        ),
        alpha=complex_floats(bound=1e3),
    )
    def test_axpy(self, cls, pair, alpha):
        xs, ys = pair
        xv, yv = make_vector(cls, xs), make_vector(cls, ys)
        xv.axpy(alpha, yv)
        assert as_numpy(xv) == pytest.approx(
            np.asarray(xs, dtype=complex) + alpha * np.asarray(ys, dtype=complex),
            rel=RTOL,
            abs=1e-300,
        )
        # axpy must not touch its argument
        assert as_numpy(yv) == pytest.approx(np.asarray(ys, dtype=complex), abs=0.0)

    @given(data=complex_vector_data())
    def test_copy_is_deep(self, cls, data):
        vec = make_vector(cls, data)
        clone = vec.copy()
        clone.set_entry(0, clone.get_entry(0) + 1.0)
        assert vec.get_entry(0) == data[0]


def make_matrix(cls, rows, cols, entries):
    """entries: dict {(i, j): complex value}, all other entries zero."""
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    for (ii, jj), value in entries.items():
        mat.set_entry(ii, jj, value)
    return mat


def matrix_as_numpy(mat):
    return np.array(
        [[mat.get_entry(ii, jj) for jj in range(mat.cols)] for ii in range(mat.rows)],
        dtype=complex,
    )


@st.composite
def complex_matrix_entries(draw, rows, cols, bound=1e3):
    return {
        (ii, jj): draw(complex_floats(bound))
        for ii in range(rows)
        for jj in range(cols)
    }


@pytest.mark.skipif(
    not MATRIX_CLASSES, reason="no complex-valued LA matrix binding available"
)
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestComplexMatrixAgainstNumpy:
    @given(
        shape=st.tuples(st.integers(1, 8), st.integers(1, 8)),
        data=st.data(),
    )
    def test_entry_roundtrip_and_matvec(self, cls, shape, data):
        rows, cols = shape
        entries = data.draw(complex_matrix_entries(rows, cols))
        mat = make_matrix(cls, rows, cols, entries)
        assert mat.rows == rows
        assert mat.cols == cols
        expected = np.zeros((rows, cols), dtype=complex)
        for (ii, jj), value in entries.items():
            expected[ii, jj] = value
        assert matrix_as_numpy(mat) == pytest.approx(expected, abs=0.0)

        # matvec: mat.dot(vec) (bound via addbind_Matrix_Vector_interaction) must match numpy
        x_values = data.draw(
            st.lists(complex_floats(bound=1e3), min_size=cols, max_size=cols)
        )
        xv = mat.vector_type()(cols, 0j)
        for jj, value in enumerate(x_values):
            xv.set_entry(jj, value)
        yv = mat.dot(xv)
        y_expected = expected @ np.asarray(x_values, dtype=complex)
        y_actual = np.array([yv.get_entry(ii) for ii in range(rows)], dtype=complex)
        assert y_actual == pytest.approx(y_expected, rel=1e-9, abs=1e-6)

    @given(
        shape=st.tuples(st.integers(1, 8), st.integers(1, 8)),
        data=st.data(),
        alpha=complex_floats(bound=1e3),
    )
    def test_scal(self, cls, shape, data, alpha):
        rows, cols = shape
        entries = data.draw(complex_matrix_entries(rows, cols))
        mat = make_matrix(cls, rows, cols, entries)
        expected = matrix_as_numpy(mat) * alpha
        mat.scal(alpha)
        assert matrix_as_numpy(mat) == pytest.approx(expected, rel=RTOL, abs=1e-300)


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
