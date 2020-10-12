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

# Same script as test_cpp.bash but without the TEST_MODULE_SUBDIR check as it does not work in Azure
# For some reason,
#[ "${TESTS_MODULE_SUBDIR}" = "None" ]
# always expands to a check like
#[ "'None'" = None ]
# and evaluates to false even if ${TEST_MODULE_SUBDIR} is set to None (both 'None' and "None" do not work)
source ${SUPERDIR}/scripts/bash/retry_command.bash
source ${OPTS}
CTEST="ctest -V --timeout ${DXT_TEST_TIMEOUT:-300} -j ${DXT_TEST_PROCS:-2}"

${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD}
${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD} test_binaries
HEADERCHECK="headercheck"

ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=print_stacktrace=1:report_error_type=1 ${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${CTEST}
${SRC_DCTRL} ${BLD} --only=${MY_MODULE} bexec ${BUILD_CMD} ${HEADERCHECK}

cp ${DUNE_BUILD_DIR}/${MY_MODULE}/${MY_MODULE//-/\/}/test/*xml ${HOME}/testresults/
