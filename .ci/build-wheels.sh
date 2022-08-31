#!/bin/bash -l
set -exu

# TODO why?
rm -rf ${DUNE_SRC_DIR}/deps/dune-uggrid

OPTS=${DUNE_SRC_DIR}/deps/config.opts/manylinux

# sets Python path, etc.
source /usr/local/bin/pybin.sh
export CCACHE_DIR=${WHEEL_DIR}/../cache
mkdir ${WHEEL_DIR}/{tmp,final} -p || true

cd ${DUNE_SRC_DIR}
DUNE_CTRL=./deps/dune-common/bin/dunecontrol
${DUNE_CTRL}  --opts=${OPTS} configure
${DUNE_CTRL}  --opts=${OPTS} make -j $(nproc --ignore 1) -l $(nproc --ignore 1)

for md in xt gdt ; do
  if [[ -d dune-${md} ]]; then
    ${DUNE_CTRL}  --opts=${OPTS} --only=dune-${md}  make -j $(nproc --ignore 1) -l $(nproc --ignore 1) bindings
    python3 -m pip wheel ${DUNE_BUILD_DIR}/dune-${md}/python -w ${WHEEL_DIR}/tmp
    # Bundle external shared libraries into the wheels
    for whl in $(ls ${WHEEL_DIR}/tmp/dune_${md}*.whl); do
        # but only in the freshly built wheels, not the downloaded dependencies
        [[ $whl == *"manylinux"* ]] || \
            python3 -m auditwheel repair --plat ${PLATFORM} $whl -w ${WHEEL_DIR}/final
    done
    # install wheel to be available for other modules
    python3 -m pip install ${WHEEL_DIR}/final/dune_${md}*.whl
  fi
done
