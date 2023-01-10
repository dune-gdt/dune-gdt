#!/bin/bash

set -euo pipefail

THISDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" && pwd -P )"
WORK_DIR=$(mktemp -d -p "${THISDIR}")

# check if tmp dir was created
if [[ ! "${WORK_DIR}" || ! -d "${WORK_DIR}" ]]; then
  echo "Could not create temp dir"
  exit 1
fi

# deletes the temp directory
function cleanup {      
  rm -rf "${WORK_DIR}"
  echo "Deleted temp working directory ${WORK_DIR}"
}

# register the cleanup function to be called on the EXIT signal
trap cleanup EXIT

python -m venv ${WORK_DIR}/venv
. ${WORK_DIR}/venv/bin/activate
python -m pip install pypi-cleanup

echo "Input test.pypi.username"
read username
echo "Input test.pypi.password"
read password

CLEANUP_CMD="pypi-cleanup --username ${username} --do-it --host https://test.pypi.org/ "
PYPI_CLEANUP_PASSWORD="${password}" ${CLEANUP_CMD}  --package dune-xt 
PYPI_CLEANUP_PASSWORD="${password}" ${CLEANUP_CMD} --package dune-gdt

