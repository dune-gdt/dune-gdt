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
"""Property test for the bound LA::MatrixInverter (dune/xt/la/matrix-inverter.hh).

Each `<Matrix>MatrixInverter` dune.xt.la binds (real and complex Common dense/sparse and Eigen
dense, discovered via discover_matrix_inverter_types) is checked by the defining property of an
inverse: A @ inv(A) == inv(A) @ A == I. The compiled dune/xt/test/la/matrixinverter_for_*.tpl tests
pin a few fixed matrices to a hand-computed expected inverse; hypothesis instead generates a fresh
well-conditioned (strictly diagonally dominant, hence invertible by Levy-Desplanques) system every
run and iterates the full MatrixInverterOptions::types() list, the same way the .tpl suite does.
"""

import numpy as np
import pytest
from hypothesis import given, settings
from hypothesis import strategies as st

from dune.xt.test.hypothesis_strategies import (
    container_field,
    dense_sparsity_pattern,
    discover_matrix_inverter_types,
)

MATRIX_CLASSES = list(discover_matrix_inverter_types())

# direct inversion of a well-conditioned matrix is accurate; leave headroom for backend round-off
ATOL = 1e-8
RTOL = 1e-6


def inverter_for(matrix_cls):
    import dune.xt.la as la

    return getattr(la, matrix_cls.__name__ + "MatrixInverter")


def numpy_dtype(matrix_cls):
    return complex if container_field(matrix_cls) == "complex" else float


def make_matrix(cls, arr):
    rows, cols = arr.shape
    mat = cls(rows, cols, dense_sparsity_pattern(rows, cols))
    scalar = complex if np.iscomplexobj(arr) else float
    for ii in range(rows):
        for jj in range(cols):
            mat.set_entry(ii, jj, scalar(arr[ii, jj]))
    return mat


def matrix_as_numpy(mat, dtype):
    return np.array(
        [[mat.get_entry(ii, jj) for jj in range(mat.cols)] for ii in range(mat.rows)],
        dtype=dtype,
    )


@st.composite
def diagonally_dominant(draw, dtype, min_size=1, max_size=5, bound=5.0):
    """A strictly diagonally dominant (hence invertible) n x n matrix of the requested dtype."""
    n = draw(st.integers(min_size, max_size))

    def scalar():
        re = draw(st.floats(-bound, bound, allow_nan=False, allow_infinity=False))
        if dtype is complex:
            im = draw(st.floats(-bound, bound, allow_nan=False, allow_infinity=False))
            return complex(re, im)
        return re

    a = np.zeros((n, n), dtype=dtype)
    for ii in range(n):
        for jj in range(n):
            if ii != jj:
                a[ii, jj] = scalar()
    for ii in range(n):
        # make the diagonal strictly dominate the off-diagonal row magnitudes
        off = float(np.sum(np.abs(a[ii, :]))) - float(np.abs(a[ii, ii]))
        a[ii, ii] = off + 1.0
    return a


def assert_is_inverse(a, inv, context):
    n = a.shape[0]
    identity = np.eye(n, dtype=inv.dtype)
    assert a @ inv == pytest.approx(identity, rel=RTOL, abs=ATOL), (
        f"{context}: A@inv != I"
    )
    assert inv @ a == pytest.approx(identity, rel=RTOL, abs=ATOL), (
        f"{context}: inv@A != I"
    )


@pytest.mark.skipif(not MATRIX_CLASSES, reason="no matrix-inverter binding available")
@pytest.mark.parametrize("cls", MATRIX_CLASSES, ids=lambda c: c.__name__)
class TestMatrixInverter:
    @settings(deadline=None)
    @given(data=st.data())
    def test_default_inverse(self, cls, data):
        dtype = numpy_dtype(cls)
        a = data.draw(diagonally_dominant(dtype))
        inverter = inverter_for(cls)(make_matrix(cls, a))
        inv = matrix_as_numpy(inverter.inverse(), dtype)
        assert inv.shape == a.shape
        assert_is_inverse(a, inv, "default inverse")

    @settings(deadline=None)
    @given(data=st.data())
    def test_every_inversion_type(self, cls, data):
        dtype = numpy_dtype(cls)
        a = data.draw(diagonally_dominant(dtype))
        inverter_cls = inverter_for(cls)
        matrix = make_matrix(cls, a)

        types = inverter_cls.types()
        assert types, "MatrixInverter advertises no types"
        for inversion_type in types:
            inv = matrix_as_numpy(inverter_cls(matrix, inversion_type).inverse(), dtype)
            assert_is_inverse(a, inv, f"inverse(type={inversion_type!r})")
            # also via the options Configuration overload
            inv_opts = matrix_as_numpy(
                inverter_cls(matrix, inverter_cls.options(inversion_type)).inverse(),
                dtype,
            )
            assert_is_inverse(a, inv_opts, f"inverse(options={inversion_type!r})")


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
