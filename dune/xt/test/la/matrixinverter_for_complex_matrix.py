# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2018 - 2019)
#   Tobias Leibner (2018 - 2021)
#
# This file is part of the dune-xt project:
# ~~~

from dune.xt.codegen import typeid_to_typedef_name as safe_name, have_eigen

matrix = [
    'EigenDenseMatrix<std::complex<double>>', 'FieldMatrix<std::complex<double>, 2, 2>',
    'CommonDenseMatrix<std::complex<double>>',
    'CommonDenseMatrix<std::complex<double>, Common::StorageLayout::dense_column_major>',
    'CommonSparseMatrix<std::complex<double>>'
]


def _ok(ft):
    if 'Eigen' in ft:
        return have_eigen(cache)
    return True


testtypes = [(safe_name('_'.join(ft)), ft) for ft in matrix if _ok(ft)]
