# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2019 - 2021)
#   René Fritze     (2018 - 2019, 2022)
#   Tobias Leibner  (2018, 2020 - 2021)
#
# This file is part of the dune-xt project:
# ~~~

dune_pybindxi_add_module(_test_grid_element grid_element.cc)
dune_pybindxi_add_module(_test_grid_intersection grid_intersection.cc)
dune_pybindxi_add_module(_test_grid_types grid_types.cc)
