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
BUILD_CMD="ninja -v -j2"

# TODO this is should be baked into the entrypoint
. /venv/bin/activate

cd /src

${DUNECONTROL} --opts=${OPTS_PATH} --only=dune-gdt bexec ${BUILD_CMD} "dxt_headercheck"

