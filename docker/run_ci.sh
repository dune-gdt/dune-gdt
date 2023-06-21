#!/bin/bash

if [[ "X${OPTS}" == "X" ]] ; then
    >&2 echo "ERROR: need to specify one of deps/config.opts/ as OPTS"
fi

# the dir containing this script is needed for mounting stuff into the container
PROJECTDIR="$(cd "$(dirname ${BASH_SOURCE[0]})/.." ;  pwd -P)"
CONTAINER=ghcr.io/dune-gdt/dune-gdt/ci
TAG=${TAG:-$(./docker/compute_ci_hash.sh)}
PORT="18$(( ( RANDOM % 10 ) ))$(( ( RANDOM % 10 ) ))$(( ( RANDOM % 10 ) ))"
DOCKER_BIN=${DOCKER:-docker}
SOURCE=${PROJECTDIR}
TARGET="/source"
BUILDDIR="${PROJECTDIR}/build-dune-gdt-ci/${OPTS}"

echo "======================================================================================"
echo "  Starting a CI docker container for dune-gdt using"
echo "    ${BUILDDIR}"
echo "  as persistent build-dir"
echo "======================================================================================"
mkdir -p ${BUILDDIR}

${DOCKER_BIN} run --rm=true --privileged=true -t -i --hostname docker \
  -e DISPLAY=$DISPLAY -v /tmp/.X11-unix:/tmp/.X11-unix \
  -e OPTS=${OPTS} \
  -e EXPOSED_PORT=$PORT -p 127.0.0.1:$PORT:$PORT \
  -v /etc/localtime:/etc/localtime:ro \
  -v "${SOURCE}":"${TARGET}" \
  -v "${BUILDDIR}":"/build/${OPTS}/dune-gdt" \
  ${CONTAINER}:${TAG} "${@}"