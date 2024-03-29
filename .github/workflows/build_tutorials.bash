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

cd /src

if [[ ${CC} == *"clang"* ]] ; then
  ASAN_LIB=$(${CC} -print-file-name=libclang_rt.asan-x86_64.so)
else
  ASAN_LIB=""
fi

ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} LD_PRELOAD=${ASAN_LIB} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} bindings
ASAN_OPTIONS=${ASAN_OPTIONS} UBSAN_OPTIONS=${UBSAN_OPTIONS} LD_PRELOAD=${ASAN_LIB} ${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} tutorials

[[ -d ${DUNE_BUILD_DIR}/testresults/tutorials/ ]] || mkdir -p ${DUNE_BUILD_DIR}/testresults/tutorials/
cp -r ${DUNE_BUILD_DIR}/dune-gdt/tutorials/html ${DUNE_BUILD_DIR}/testresults/tutorials/