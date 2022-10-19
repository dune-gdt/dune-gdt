# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2013 - 2018)
#   René Fritze     (2012, 2015 - 2016, 2018 - 2019, 2022)
#   Tobias Leibner  (2018, 2020 - 2021)
#   f_albr01        (2010)
# ~~~

enable_testing()

add_subdirectory(xt)
add_subdirectory(pybindxi)
install(
  DIRECTORY xt
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/
  FILES_MATCHING
  PATTERN "*.hh")
add_subdirectory(gdt)

# TODO find a better place for these files
dune_symlink_to_source_files(FILES gmsh_2d_simplices.msh dgf_1d_interval.dgf dgf_2d_interval.dgf dgf_3d_interval.dgf
                             dgf_4d_interval.dgf)

finalize_test_setup()
