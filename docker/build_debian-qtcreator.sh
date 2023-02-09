#!/bin/bash

# the dir containing this script is the base
BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"

cd "${BASEDIR}"
docker build --no-cache -t dune-gdt-debian-qtcreator:latest -f debian-qtcreator/Dockerfile debian-qtcreator/
# docker build -t oasys-solver-suite-ubuntu-lts-base:latest -f ubuntu-lts-base/Dockerfile ubuntu-lts-base/

