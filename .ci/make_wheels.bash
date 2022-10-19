#!/usr/bin/env bash
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
# ~~~

set -exo pipefail

# otherwise versioneer fails on mounted source directories in CI
git config --global --add safe.directory '*'

THISDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd -P )"
md=gdt

[[ -f /usr/local/bin/pybin.sh ]] && source /usr/local/bin/pybin.sh

python3 -m pip install -q twine
PIP_CONFIG_FILE=${PIP_CONFIG_FILE:-${HOME}/.config/pip/pip.conf}
[[ -d $(dirname ${PIP_CONFIG_FILE})  ]]  || mkdir -p $(dirname ${PIP_CONFIG_FILE})
sed "s;PYPI_INDEX_URL;${GITLAB_PYPI}/simple;g" ${THISDIR}/pip.conf > ${PIP_CONFIG_FILE}

${THISDIR}/build-wheels.sh ${md}

echo '************************************'
echo Wheels are in ${WHEEL_DIR}/final
python3 -m twine check ${WHEEL_DIR}/final/*${md}*.whl
echo '************************************'

du -sch ${DUNE_BUILD_DIR}/*