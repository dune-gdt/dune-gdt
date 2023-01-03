# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017)
#   René Fritze     (2017 - 2019)
#   Tobias Leibner  (2017, 2019 - 2020)
# ~~~

from dune.xt.codegen import typeid_to_typedef_name as safe_name, have_eigen

matrix = ['EigenDenseMatrix<double>', 'FieldMatrix<double, 6, 6>']
field = ['double', 'double']
complex_matrix = ['EigenDenseMatrix<std::complex<double>>', 'FieldMatrix<std::complex<double>, 6, 6>']
real_matrix = ['EigenDenseMatrix<double>', 'FieldMatrix<double, 6, 6>']


def _ok(ft):
    if 'Eigen' in ft[0]:
        return have_eigen(cache)  # noqa: F821
    return True


testtypes = [(safe_name('_'.join(ft)), *ft) for ft in zip(matrix, field, complex_matrix, real_matrix) if _ok(ft)]
