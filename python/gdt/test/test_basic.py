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
