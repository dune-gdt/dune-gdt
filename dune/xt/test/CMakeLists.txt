# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2014 - 2016)
#   René Fritze     (2012 - 2016, 2019, 2022)
#   Tobias Leibner  (2015 - 2016, 2019 - 2021)
#
# This file is part of the dune-xt project:
# ~~~

enable_testing()

dependencycheck(${xtcommon})

include_directories(SYSTEM ${CMAKE_CURRENT_SOURCE_DIR}/gtest)
dune_library_add_sources(gtest_dune_xt SOURCES common.cxx gtest/gtest-all.cc)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gtest
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/xt/test/
  FILES_MATCHING
  PATTERN "*")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.hxx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/xt/test/)

add_subdir_tests(common)
add_subdir_tests(grid)
add_subdir_tests(la)
add_subdir_tests(functions)
