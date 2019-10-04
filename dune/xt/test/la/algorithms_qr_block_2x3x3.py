# ~~~
# This file is part of the dune-xt project:
#   https://github.com/dune-community/dune-xt
# Copyright 2009-2018 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2018 - 2019)
#   Tobias Leibner (2018 - 2019)
# ~~~

from algorithms import la_types, testtypes_creator

common_types = [
    f.split('_') for f in [
        'FieldMatrix_FieldVector_FieldVector_double,6,6', 'BlockedFieldMatrix_FieldVector_FieldVector_double,2,3,3',
        'BlockedFieldMatrix_BlockedFieldVector_BlockedFieldVector_double,2,3,3'
    ]
]

dune_types = [
    f.split('_')
    for f in ['FieldMatrix_FieldVector_FieldVector_double,6,6', 'DynamicMatrix_DynamicVector_DynamicVector_double']
]

testtypes = testtypes_creator(la_types, common_types, dune_types, cache)