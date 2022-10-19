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

DUNECONTROL=/src/deps/dune-common/bin/dunecontrol
BUILD_CMD="ninja -v -j1"

# TODO this is should be baked into the entrypoint
. /venv/bin/activate

# TODO This should be automatic from module deps
python -m pip install pytest pytest-cov hypothesis codecov

cd /src

if [[ ${CC} == *"clang"* ]] ; then
  ASAN_LIB=$(${CC} -print-file-name=libclang_rt.asan-x86_64.so)
else
  ASAN_LIB=""
fi

ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} LD_PRELOAD=${ASAN_LIB} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} bindings
ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} LD_PRELOAD=${ASAN_LIB} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} test_python

[[ -d ${DUNE_BUILD_DIR}/testresults/ ]] || mkdir ${DUNE_BUILD_DIR}/testresults/
cp ${DUNE_BUILD_DIR}/dune-gdt/pytest_results*.xml ${DUNE_BUILD_DIR}/testresults/

ccache --print-stats