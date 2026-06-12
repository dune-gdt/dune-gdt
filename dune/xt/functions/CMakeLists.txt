# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2016 - 2017)
#   René Fritze     (2016, 2018 - 2019)
#   Tim Keil        (2018)
#   Tobias Leibner  (2016, 2018 - 2020)
# ~~~

set(_lib_dune_xt_functions_sources expression/engine.cc)
dune_library_add_sources(dunext SOURCES ${_lib_dune_xt_functions_sources})

# The expression functions are backed by ExprTk, a header-only math expression parser pulled in via
# vcpkg (see vcpkg.json). Its vcpkg port ships only the header without a CMake config, so we locate
# exprtk.hpp explicitly and add it as a SYSTEM include (the header is large and would otherwise emit
# many warnings) to the dunext sources that need it (expression/engine.cc).
find_path(EXPRTK_INCLUDE_DIR NAMES exprtk.hpp REQUIRED)
target_include_directories(dunext SYSTEM PRIVATE $<BUILD_INTERFACE:${EXPRTK_INCLUDE_DIR}>)
