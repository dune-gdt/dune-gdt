# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2024)
# ~~~

# dune.gdt.basic is a thin facade: it re-exports a curated set of names from the compiled `dune.gdt._*` binding modules
# (and, when k3d is available, visualize_function) for convenient `from dune.gdt.basic import ...` access. There is no
# pure-Python logic to test, so this is a light smoke test that the facade imports and exposes a representative subset of
# its re-exports.
#
# It is skipped when the facade cannot be imported. This covers a bare source checkout (the binding modules are absent,
# raising ModuleNotFoundError) as well as build configurations that do not export every re-exported symbol -- e.g. the
# debug build omits the ISTL operators, so `from dune.gdt._operators_matrix_based_factory import IstlSparseMatrixOperator`
# raises a plain ImportError partway through importing the facade. pytest.importorskip only auto-skips on
# ModuleNotFoundError for the requested module, so exc_type=ImportError is needed to skip on that nested failure too.

import pytest

from dune.xt.common.config import config

basic = pytest.importorskip("dune.gdt.basic", exc_type=ImportError)


@pytest.mark.parametrize(
    "name",
    [
        "DiscreteFunction",
        "BilinearForm",
        "Operator",
        "ContinuousLagrangeSpace",
        "default_interpolation",
        "make_element_sparsity_pattern",
    ],
)
def test_basic_reexports_expected_names(name):
    assert hasattr(basic, name)


def test_basic_reexports_visualize_function_iff_have_k3d():
    # Regression test for #393: k3d is now in the ctest `test` dependency group, so this branch
    # (dune/gdt/basic.py:3) is exercised both ways for the first time -- previously HAVE_K3D was
    # permanently False under ctest and only the negative case was ever reached.
    assert hasattr(basic, "visualize_function") == config.HAVE_K3D
