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
"""Scenario 1: property-based tests of the C++ LA vector containers against a numpy oracle.

The C++ side covers this via dune/xt/test/la/container_vector.tpl, which dxt_code_generation
expands into one translation unit per (container backend x field type) and compiles each.
Here the same cross product is a runtime pytest parametrization over the already-compiled
binding classes, and hypothesis supplies the payloads that the .tpl tests hard-code.
"""

import subprocess
import sys

import numpy as np
import pytest
from hypothesis import given
from hypothesis import strategies as st

import dune.xt.la as la
from dune.xt.test.hypothesis_strategies import finite_floats, vector_data

# The available backends depend on the build configuration (e.g. Eigen may be disabled), so
# collect whatever this build provides instead of hard-coding the list.
VECTOR_CLASSES = [
    cls
    for cls in (
        getattr(la, name, None)
        for name in ("CommonVector", "IstlVector", "EigenVector")
    )
    if cls is not None
]

# relative tolerance for comparisons against numpy: both sides accumulate in double, but not
# necessarily in the same order, so exact equality only holds for order-independent quantities
RTOL = 1e-12


def make_vector(cls, values):
    vec = cls(len(values), 0.0)
    for ii, value in enumerate(values):
        vec.set_entry(ii, float(value))
    return vec


def as_numpy(vec):
    return np.array([vec.get_entry(ii) for ii in range(len(vec))])


@pytest.mark.parametrize("cls", VECTOR_CLASSES, ids=lambda c: c.__name__)
class TestVectorAgainstNumpy:
    @given(data=vector_data())
    def test_entry_roundtrip(self, cls, data):
        vec = make_vector(cls, data)
        assert vec.size == len(data)
        assert len(vec) == len(data)
        assert as_numpy(vec) == pytest.approx(np.asarray(data), abs=0.0)

    @given(data=vector_data())
    def test_norms(self, cls, data):
        vec = make_vector(cls, data)
        arr = np.asarray(data)
        assert vec.l1_norm() == pytest.approx(np.linalg.norm(arr, 1), rel=RTOL)
        assert vec.l2_norm() == pytest.approx(np.linalg.norm(arr, 2), rel=RTOL)
        assert vec.sup_norm() == pytest.approx(np.linalg.norm(arr, np.inf), rel=RTOL)

    @given(data=vector_data(), scalar=finite_floats(bound=1e3))
    def test_scal(self, cls, data, scalar):
        vec = make_vector(cls, data)
        vec.scal(scalar)
        assert as_numpy(vec) == pytest.approx(
            scalar * np.asarray(data), rel=RTOL, abs=1e-300
        )

    @given(
        pair=st.integers(1, 64).flatmap(
            lambda n: st.tuples(vector_data(n, n), vector_data(n, n))
        )
    )
    def test_dot_matches_numpy_and_is_symmetric(self, cls, pair):
        xs, ys = pair
        xv, yv = make_vector(cls, xs), make_vector(cls, ys)
        expected = float(np.dot(xs, ys))
        # dot against numpy needs an absolute escape hatch: cancellation can make the exact
        # result arbitrarily smaller than the intermediate terms
        abs_tol = 1e-9 * max(
            1.0, float(np.sum(np.abs(np.asarray(xs) * np.asarray(ys))))
        )
        assert xv.dot(yv) == pytest.approx(expected, rel=RTOL, abs=abs_tol)
        assert xv.dot(yv) == yv.dot(xv)

    @given(
        pair=st.integers(1, 64).flatmap(
            lambda n: st.tuples(vector_data(n, n), vector_data(n, n))
        ),
        alpha=finite_floats(bound=1e3),
    )
    def test_axpy(self, cls, pair, alpha):
        xs, ys = pair
        xv, yv = make_vector(cls, xs), make_vector(cls, ys)
        xv.axpy(alpha, yv)
        assert as_numpy(xv) == pytest.approx(
            np.asarray(xs) + alpha * np.asarray(ys), rel=RTOL, abs=1e-300
        )
        # axpy must not touch its argument
        assert as_numpy(yv) == pytest.approx(np.asarray(ys), abs=0.0)

    @given(data=vector_data())
    def test_statistics(self, cls, data):
        vec = make_vector(cls, data)
        arr = np.asarray(data)
        assert vec.mean() == pytest.approx(
            arr.mean(), rel=RTOL, abs=1e-9 * float(np.abs(arr).sum() + 1)
        )
        assert vec.standard_deviation() == pytest.approx(
            arr.std(), rel=1e-9, abs=1e-9 * float(np.abs(arr).max() + 1)
        )

    @given(data=vector_data())
    def test_almost_equal_is_reflexive_and_detects_copies(self, cls, data):
        vec = make_vector(cls, data)
        assert vec.almost_equal(vec)
        assert vec.almost_equal(vec.copy())


# Iterating an *empty* vector of any backend segfaults with the current bindings
# (`list(la.CommonVectorSizeT(0, 0))` is a minimal reproducer; nonempty vectors iterate
# fine). Found by hypothesis shrinking the grid-provider partition test down to a
# single-element grid, whose inner_intersection_indices() returns an empty
# CommonVectorSizeT. Exercised in a subprocess so the crash fails this test instead of
# killing the whole pytest process; once the binding is fixed the assertion still runs.
@pytest.mark.parametrize(
    "cls_name",
    ["CommonVector", "IstlVector", "EigenVector", "CommonVectorSizeT"],
)
def test_iterating_empty_vector(cls_name):
    if getattr(la, cls_name, None) is None:
        pytest.skip(f"{cls_name} not available in this build")
    code = f"import dune.xt.la as la\nassert list(la.{cls_name}(0, 0)) == []\n"
    result = subprocess.run(
        [sys.executable, "-c", code], capture_output=True, text=True
    )
    if result.returncode < 0:
        pytest.xfail(
            f"iterating an empty {cls_name} crashes (signal {-result.returncode}); "
            "known upstream binding bug found by this test suite"
        )
    assert result.returncode == 0, result.stderr


if __name__ == "__main__":
    from dune.xt.test.base import runmodule

    runmodule(__file__)
