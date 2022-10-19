#!/bin/sh
#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2016 - 2017)
#   René Fritze     (2012, 2015, 2018 - 2019)
#   Tobias Leibner  (2016, 2020 - 2021)
#
# This file is part of the dune-xt project:
# ~~~

make grid_output_pgf
./grid_output_pgf
find . -name "*.tex" | xargs echo pdflatex
