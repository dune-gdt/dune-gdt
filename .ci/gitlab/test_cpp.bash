#!/bin/bash
#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2022)
#
# This file is part of the dune-xt project:
# ~~~

set -ex

THIS_DIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P )"
OPTS_PATH=${THIS_DIR}/../../deps/config.opts/${CONFIG_OPTS}
source ${OPTS_PATH}
set -u

CTEST="ctest -V --timeout ${DXT_TEST_TIMEOUT:-300} -j ${DXT_TEST_PROCS:-2}"

DUNECONTROL=/src/deps/dune-common/bin/dunecontrol

# TODO this is should be baked into the entrypoint
. /venv/bin/activate
BUILD_CMD="ninja -v -j1"

cd /src

${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} test_binaries

ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${CTEST}

[[ -d ${DUNE_BUILD_DIR}/testresults/ ]] || mkdir ${DUNE_BUILD_DIR}/testresults/
cp ${DUNE_BUILD_DIR}/dune-gdt/dune/*xml ${DUNE_BUILD_DIR}/testresults/

ccache --print-stats