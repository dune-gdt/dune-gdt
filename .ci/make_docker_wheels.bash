#!/usr/bin/env bash

# this script is intended to be run manually

set -exo pipefail

THISDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P )"

cd "${THISDIR}/.."

# for rootless/podman execution set both to 0
LOCAL_USER="${LOCAL_USER:-$USER}"
LOCAL_UID="${LOCAL_UID:-$(id -u)}"
LOCAL_GID="${LOCAL_GID:-$(id -g)}"
PYTHON_VERSION="${GDT_PYTHON_VERSION:-3.13}"
ML_TAG=2025.04.19-1
PLATFORM=manylinux_2_28_x86_64
ML_IMAGE_BASE="quay.io/pypa/${PLATFORM}"
WHEELDIR_RELATIVE=build/wheelhouse
WHEEL_DIR_ABSOLUTE="${THISDIR}/../${WHEELDIR_RELATIVE}"
mkdir -p "${WHEEL_DIR_ABSOLUTE}" || true

set -euxo pipefail

IMAGE="${ML_IMAGE_BASE}:${ML_TAG}"
TEST_IMAGE="docker.io/python:${PYTHON_VERSION}-slim"
# check if we a have TTY first, else docker run would throw an error
if [ -t 1 ] ; then
  DT="-t"
else
  DT=""
fi

[[ -e "${THISDIR}/docker" ]] || mkdir -p "${THISDIR}/docker"
export ENV_FILE="${THISDIR}/docker/env"
python3 ./deps/scripts/python/make_env_file.py

docker pull -q "${IMAGE}"
# this can happen in the background while we build stuff
docker pull -q "${TEST_IMAGE}" &

# make sure we only have one whl per module _after_ the build
for md in xt gdt; do
  if [ $(ls -1q "${WHEEL_DIR_ABSOLUTE}"/final/dune_"${md}"*.whl 2>/dev/null | wc -l) -gt 1 ]; then
    echo "Error: More than one dune_${md} wheel file found in the final wheelhouse directory." >&2
    exit 1
  fi
done

# default command is "build-wheels.sh"
# this deletes testtols and uggrid source dirs
DOCKER_RUN="docker run ${DT} --env-file=${ENV_FILE} -e DUNE_SRC_DIR=/home/dxt/src -v ${THISDIR}/../:/home/dxt/src \
  -e LOCAL_USER=${LOCAL_USER} -e LOCAL_GID=${LOCAL_GID} -e LOCAL_UID=${LOCAL_UID} \
  -e WHEELDIR_RELATIVE=${WHEELDIR_RELATIVE} -e PYTHON_VERSION=${PYTHON_VERSION} -e PLATFORM=${PLATFORM} \
  -i ${IMAGE}"

${DOCKER_RUN} /home/dxt/src/.ci/build-wheels.sh

# wait for pull to finish
wait
# makes sure wheels are importable

for md in dune.xt dune.gdt; do
  # check if wheels are importable
  docker run ${DT} -v "${WHEEL_DIR_ABSOLUTE}"/final:/wheelhouse:ro -i "${TEST_IMAGE}" \
    bash -c "pip install /wheelhouse/dune* && python -c 'from ${md} import *'"
done

echo '************************************'
echo "Wheels are in ${WHEEL_DIR_ABSOLUTE}/final"
