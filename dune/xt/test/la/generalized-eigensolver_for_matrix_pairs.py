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

from dune.xt.codegen import have_eigen
from dune.xt.codegen import typeid_to_typedef_name as safe_name

# One entry per matrix backend under test: how to spell the type for a given scalar, and whether it needs eigen.
# Spelled as format strings rather than as the four parallel real/field/complex/real lists the sibling eigensolver
# configurations use, so that the real and the complex spelling of a backend cannot drift apart.
MATRIX_TEMPLATES = [
    ("EigenDenseMatrix<{scalar}>", True),
    ("FieldMatrix<{scalar}, 2, 2>", False),
    ("CommonDenseMatrix<{scalar}>", False),
    ("CommonSparseMatrix<{scalar}>", False),
]


def _test_type(template):
    """(matrix, field, complex matrix, real matrix), the tuple the templates expect."""
    real = template.format(scalar="double")
    return real, "double", template.format(scalar="std::complex<double>"), real


testtypes = [
    (safe_name("_".join(_test_type(template))), *_test_type(template))
    for template, needs_eigen in MATRIX_TEMPLATES
    if not needs_eigen or have_eigen(cache)  # noqa: F821
]
