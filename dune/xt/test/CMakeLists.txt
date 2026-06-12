# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2014 - 2016)
#   René Fritze     (2012 - 2016, 2019)
#   Tobias Leibner  (2015 - 2016, 2019 - 2020)
# ~~~

enable_testing()

dependencycheck(${xtcommon})

# GoogleTest is provided by vcpkg (see vcpkg.json, find_package(GTest) in the top-level CMakeLists.txt). gtest_dune_xt
# bundles our test helpers (common.cxx) and links GTest publicly so that all test binaries linking gtest_dune_xt pick up
# the GoogleTest headers and libraries.
dune_library_add_sources(gtest_dune_xt SOURCES common.cxx)
target_link_libraries(gtest_dune_xt PUBLIC GTest::gtest)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.hxx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/xt/test/)

add_subdir_tests(common)
add_subdir_tests(grid)
add_subdir_tests(la)
add_subdir_tests(functions)
