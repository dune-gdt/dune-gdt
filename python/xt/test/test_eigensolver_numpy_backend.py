# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
# ~~~
"""The NumPy eigen-solver backend, exercised from inside a running interpreter.

dune-xt reaches NumPy through Dune::PybindXI::GlobalInterpreter(), which used to *embed* an interpreter
unconditionally. Doing that from inside Python crashed: pybind11 rejects a second interpreter via pybind11_fail(),
whose assert(!PyErr_Occurred()) touches the C-API, and the bindings release the GIL around grid walks, so there was no
thread state to touch. These tests pin the fixed behaviour -- the running interpreter is used, and the backend works
from Python rather than being silently unavailable.

The bindings under test are test-only (python/xt/dune/xt/test/eigensolver_numpy.cc): the FieldMatrix eigen solver,
which is the only one with a numpy branch, is not part of the public bindings.
"""

import math

import pytest

from dune.xt.test._test_eigensolver_numpy import (
    eigenvalues,
    interpreter_is_running,
    numpy_eigensolver_available,
    types,
)

# a non-symmetric matrix, so its eigenvectors are not orthogonal and a transposed eigenvector matrix could not pass
# the A = V diag(lambda) V^-1 assertion that the solver runs by default
NON_SYMMETRIC = [[1.0, 2.0], [0.0, 3.0]]
NON_SYMMETRIC_EIGENVALUES = [1.0, 3.0]


def test_interpreter_is_detected_as_running():
    """We are called from Python, so dune-xt must not try to embed an interpreter of its own."""
    assert interpreter_is_running()


def test_numpy_backend_is_available_from_python():
    """numpy is importable here by construction -- we are running in it."""
    assert numpy_eigensolver_available()


def test_numpy_is_offered_among_the_types():
    assert "numpy" in types()


def test_numpy_is_not_the_default_backend():
    """types()[0] picks the backend when none is named; numpy must stay opt-in."""
    assert types()[0] != "numpy"


@pytest.mark.parametrize("solver_type", ["numpy", "lapack", "eigen", "shifted_qr"])
def test_backend_computes_correct_eigenvalues(solver_type):
    """Runs with the GIL released, as a grid walk would.

    Also covers the eigenvector convention: assert_eigendecomposition is on by default, so a wrong one would fail here
    rather than silently returning plausible eigenvalues.
    """
    if solver_type not in types():
        pytest.skip(f"backend {solver_type} not available in this build")
    computed = sorted(eigenvalues(NON_SYMMETRIC, solver_type))
    assert len(computed) == len(NON_SYMMETRIC_EIGENVALUES)
    for actual, expected in zip(computed, NON_SYMMETRIC_EIGENVALUES):
        assert math.isclose(actual, expected, abs_tol=1e-12)


def test_numpy_backend_does_not_reconfigure_the_session():
    """The backend turns warnings and floating point errors into exceptions; that must not leak out of the call."""
    import warnings

    import numpy as np

    before_filters = list(warnings.filters)
    before_errstate = np.geterr()
    eigenvalues(NON_SYMMETRIC, "numpy")
    assert np.geterr() == before_errstate
    assert list(warnings.filters) == before_filters


def test_numpy_backend_is_reentrant():
    """The probe is cached and the GIL is taken per call, so repeated use must keep working."""
    for _ in range(5):
        assert len(eigenvalues(NON_SYMMETRIC, "numpy")) == 2
