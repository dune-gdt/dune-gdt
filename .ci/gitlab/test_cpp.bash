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

THIS_DIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P )"
OPTS_PATH=${THIS_DIR}/../../deps/config.opts/${CONFIG_OPTS}
source ${OPTS_PATH}
set -u

CTEST="ctest -V --timeout ${DXT_TEST_TIMEOUT:-300} -j ${DXT_TEST_PROCS:-2} -L ${TESTS_MODULE_SUBDIR}"
# BUILD_CMD="ninja -v -j2 -k 10000"
BUILD_CMD="ninja -v -j2"
DUNECONTROL=dunecontrol

# TODO this is should be baked into the entrypoint
. /venv/bin/activate

${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt all
${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD}
if [ "${TESTS_MODULE_SUBDIR}" = "gdt" ] ; then
  # TODO this builds too much
  ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} test_binaries
else
  ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} ${TESTS_MODULE_SUBDIR}_test_binaries
fi

ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${CTEST}

# TODO
# cp ${DUNE_BUILD_DIR}/dune-gdt/${MY_MODULE//-/\/}/test/*xml ${HOME}/testresults/
