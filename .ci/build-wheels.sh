#!/bin/bash -l
set -exuo pipefail

# TODO: should these have come from outside?
PYTHON_VERSION=${GDT_PYTHON_VERSION:-3.13}
WHEEL_DIR=${DUNE_SRC_DIR}/wheelhouse

export CCACHE_DIR=${WHEEL_DIR}/cache
mkdir ${WHEEL_DIR}/{tmp,final} -p || true

git config --global --add safe.directory ${DUNE_SRC_DIR}

yum install -y curl zip unzip tar

python${PYTHON_VERSION} -m venv ${WHEEL_DIR}/venv
. ${WHEEL_DIR}/venv/bin/activate
python${PYTHON_VERSION} -m pip install auditwheel wheel build
cd ${DUNE_SRC_DIR}

cmake --preset release
cmake --build --preset release --target bindings -- $(nproc --ignore 1) -l $(nproc --ignore 1)

DUNE_BUILD_DIR=${DUNE_SRC_DIR}/build/release
python${PYTHON_VERSION} -m pip wheel ${DUNE_BUILD_DIR}/dune-gdt/python/xt/ -w ${WHEEL_DIR}/tmp
# xt is an exact-version dependency of gdt -> needs `--find-links`
python${PYTHON_VERSION} -m pip install ${WHEEL_DIR}/tmp/dune.xt*whl
python${PYTHON_VERSION} -m pip wheel ${DUNE_BUILD_DIR}/dune-gdt/python/gdt/ -w ${WHEEL_DIR}/tmp --find-links ${WHEEL_DIR}/tmp/
# Bundle external shared libraries into the wheels
python${PYTHON_VERSION} -m auditwheel repair --plat ${PLATFORM} ${WHEEL_DIR}/tmp/*xt*.whl -w ${WHEEL_DIR}/final
python${PYTHON_VERSION} -m auditwheel repair --plat ${PLATFORM} ${WHEEL_DIR}/tmp/*gdt*.whl -w ${WHEEL_DIR}/final

deactivate
ccache -s