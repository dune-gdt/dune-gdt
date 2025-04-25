#!/bin/bash -l
set -exuo pipefail

WHEEL_DIR="${DUNE_SRC_DIR}/${WHEELDIR_RELATIVE}"
# Python version might have qoutes around it
PYTHON_BIN="python${PYTHON_VERSION//\"/}"

export CCACHE_DIR="${WHEEL_DIR}/cache"
# Create final wheel dir, but not tmp here
mkdir -p "${WHEEL_DIR}/final" || true

# Create a temporary directory for wheel building
TMP_WHEEL_DIR=$(mktemp -d -p "${WHEEL_DIR}" tmp.XXXXXXXXXX)

cleanup() {
    rm -rf "${TMP_WHEEL_DIR}"
}
trap cleanup EXIT

# otherwise versioneer fails on mounted source directories in CI
git config --global --add safe.directory "${DUNE_SRC_DIR}"

yum install -y curl zip unzip tar ccache
pushd /usr/local/bin/
for ii in cc c++ cpp g++ gcc mpicc mpic++ mpicxx ; do ln -s "$(which ccache)" "$ii"; done
popd

# some of our vcpkg deps don't build with cmake 4 yet
# the container's cmake is managed with pipx already
pipx install --force "cmake<4"

"${PYTHON_BIN}" -m venv "${WHEEL_DIR}/venv"
# shellcheck disable=SC1091
. "${WHEEL_DIR}/venv/bin/activate"
"${PYTHON_BIN}" -m pip install auditwheel wheel build
cd "${DUNE_SRC_DIR}"

cmake --preset release -DDXT_DONT_LINK_PYTHON_LIB=1
cmake --build --preset release --target bindings -- -j "$(nproc --ignore 1)" -l "$(nproc --ignore 1)"

DUNE_BUILD_DIR="${DUNE_SRC_DIR}/build/release"
"${PYTHON_BIN}" -m pip wheel "${DUNE_BUILD_DIR}/python/xt/" -w "${TMP_WHEEL_DIR}"
# xt is an exact-version dependency of gdt -> needs `--find-links`
"${PYTHON_BIN}" -m pip install "${TMP_WHEEL_DIR}"/dune_xt*whl
"${PYTHON_BIN}" -m pip wheel "${DUNE_BUILD_DIR}/python/gdt/" -w "${TMP_WHEEL_DIR}" --find-links "${TMP_WHEEL_DIR}/"
# Bundle external shared libraries into the wheels
"${PYTHON_BIN}" -m auditwheel repair --plat "${PLATFORM}" "${TMP_WHEEL_DIR}"/*xt*.whl -w "${WHEEL_DIR}/final"
"${PYTHON_BIN}" -m auditwheel repair --plat "${PLATFORM}" "${TMP_WHEEL_DIR}"/*gdt*.whl -w "${WHEEL_DIR}/final"

deactivate
ccache -s
# cleanup will be called automatically by trap