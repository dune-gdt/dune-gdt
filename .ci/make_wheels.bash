#!/usr/bin/env bash

set -exo pipefail


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