#!/bin/bash -l
set -exuo pipefail

# TODO why?
rm -rf ${DUNE_SRC_DIR}/deps/dune-uggrid

OPTS=${DUNE_SRC_DIR}/deps/config.opts/manylinux

# sets Python path, etc.
source /usr/local/bin/pybin.sh
export CCACHE_DIR=${WHEEL_DIR}/../cache
mkdir ${WHEEL_DIR}/{tmp,final} -p || true

python3 -m venv ${WHEEL_DIR}/venv
. ${WHEEL_DIR}/venv/bin/activate
python3 -m pip install auditwheel wheel build

cd ${DUNE_SRC_DIR}
DUNE_CTRL=./deps/dune-common/bin/dunecontrol
${DUNE_CTRL} --opts=${OPTS} all
${DUNE_CTRL} --opts=${OPTS} make -j $(nproc --ignore 1) -l $(nproc --ignore 1)


${DUNE_CTRL} --opts=${OPTS} --only=dune-gdt  make -j $(nproc --ignore 1) -l $(nproc --ignore 1) bindings
python3 -m pip wheel ${DUNE_BUILD_DIR}/dune-gdt/python -w ${WHEEL_DIR}/tmp
# Bundle external shared libraries into the wheels
python3 -m auditwheel repair --plat ${PLATFORM} ${WHEEL_DIR}/tmp/*gdt*.whl -w ${WHEEL_DIR}/final

deactivate
