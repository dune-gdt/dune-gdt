#!/bin/bash

set -euo pipefail

echo "Input test.pypi.username"
read -r username
echo "Input the real test.pypi.password, NOT A TOKEN"
read -r password

CLEANUP_CMD="uvx pypi-cleanup --username ${username} --do-it --host https://test.pypi.org/  "
PYPI_CLEANUP_PASSWORD="${password}" ${CLEANUP_CMD} --package dune-xt
PYPI_CLEANUP_PASSWORD="${password}" ${CLEANUP_CMD} --package dune-gdt

