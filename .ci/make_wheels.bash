#!/usr/bin/env bash

set -exo pipefail

# otherwise versioneer fails on mounted source directories in CI
git config --global --add safe.directory '*'

THISDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd -P )"

# in case this script runs inside the manylinux container, this uses the right python
[[ -f /usr/local/bin/pybin.sh ]] && source /usr/local/bin/pybin.sh

python3 -m pip install -q twine

${THISDIR}/build-wheels.sh

echo '************************************'
echo Wheels are in ${WHEEL_DIR}/final
python3 -m twine check ${WHEEL_DIR}/final/*.whl
echo '************************************'

du -sch ${DUNE_BUILD_DIR}/*