# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2013, 2016 - 2018)
#   René Fritze     (2016 - 2018, 2022)
#   Tobias Leibner  (2016 - 2019, 2021)
#
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license
#          with "runtime exception" (http://www.dune-project.org/license.html
#   Felix Schindler (2013, 2016 - 2018
#   René Fritze     (2016 - 2018
#   Tobias Leibner  (2016 - 2018
# ~~~

set(GDT_TEST_SUBDIRS
    burgers
    discretefunction
    instationary-eocstudies
    integrands
    interpolations
    inviscid-compressible-flow
    linear-transport
    misc
    operators
    projections
    prolongations
    spaces
    stationary-eocstudies
    stationary-heat-equation
    stokes)

foreach(subdir ${GDT_TEST_SUBDIRS})
  add_subdir_tests(${subdir})
endforeach()
