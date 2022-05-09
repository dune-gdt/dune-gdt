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

set -ex

OPTS_PATH=./deps/config.opts/${OPTS}
source ${OPTS_PATH}
set -u
CTEST="ctest -V --timeout ${DXT_TEST_TIMEOUT:-300} -j ${DXT_TEST_PROCS:-2}"
# BUILD_CMD="ninja -v -j2 -k 10000"
BUILD_CMD="ninja -v -j2"

DUNECONTROL=dunecontrol
${DUNECONTROL} --opts=${OPTS_PATH} --only=${MY_MODULE} bexec ${BUILD_CMD}
if [ "${TESTS_MODULE_SUBDIR}" = "gdt" ] ; then
  ${DUNECONTROL} --opts=${OPTS_PATH} --only=${MY_MODULE} bexec ${BUILD_CMD} test_binaries
else
  ${DUNECONTROL} --opts=${OPTS_PATH} --only=${MY_MODULE} bexec ${BUILD_CMD} ${TESTS_MODULE_SUBDIR}_test_binaries
  CTEST="${CTEST} -L ${TESTS_MODULE_SUBDIR}"
fi

ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} ${DUNECONTROL} --opts=${OPTS_PATH} --only=${MY_MODULE} bexec ${CTEST}

# TODO
# cp ${DUNE_BUILD_DIR}/${MY_MODULE}/${MY_MODULE//-/\/}/test/*xml ${HOME}/testresults/
