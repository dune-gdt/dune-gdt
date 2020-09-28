#!/bin/bash
#
# ~~~
# This file is part of the dune-xt project:
#   https://github.com/dune-community/dune-xt
# Copyright 2009-2020 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2019)
#   Tobias Leibner (2019 - 2020)
# ~~~

set -e
set -x

source ${SUPERDIR}/scripts/bash/retry_command.bash

if [[ ${CC} == *"clang"* ]] ; then
  ASAN_LIB=$(${CC} -print-file-name=libclang_rt.asan-x86_64.so)
else
  ASAN_LIB=""
fi

ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:report_error_type=1 LD_PRELOAD=${ASAN_LIB} ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD}
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:report_error_type=1 LD_PRELOAD=${ASAN_LIB} ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD} bindings
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:report_error_type=1 LD_PRELOAD=${ASAN_LIB} ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD} test_python

cp ${DUNE_BUILD_DIR}/${MY_MODULE}/python/pytest_results.xml ${HOME}/testresults/
