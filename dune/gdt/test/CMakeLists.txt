# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license
#          with "runtime exception" (http://www.dune-project.org/license.html
# Authors:
#   Felix Schindler (2013, 2016 - 2018
#   René Fritze     (2016 - 2018
#   Tobias Leibner  (2016 - 2018
# ~~~

# Only subdirs that contain test sources (*.cc/*.tpl) are listed here. The header-only eocstudies directories
# (instationary-eocstudies, stationary-eocstudies) provide base.hh helpers included by tests in other subdirs, and
# projections/prolongations currently hold no tests; listing any of them would trigger add_subdir_tests author warnings
# about missing sources.
set(GDT_TEST_SUBDIRS
    burgers
    discretefunction
    finite-elements
    integrands
    interpolations
    inviscid-compressible-flow
    linear-transport
    misc
    operators
    spaces
    stationary-heat-equation
    stokes)

foreach(subdir ${GDT_TEST_SUBDIRS})
  add_subdir_tests(${subdir})
endforeach()
