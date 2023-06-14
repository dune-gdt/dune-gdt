#!/bin/bash

# the dir containing this script is the base
BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"

cd "${BASEDIR}"
docker build -t ghcr.io/dune-gdt/dune-gdt/dev:latest -f dev/Dockerfile dev/
