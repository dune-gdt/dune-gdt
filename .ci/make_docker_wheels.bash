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
  # shellcheck disable=SC2012
  if [ "$(ls -1q "${WHEEL_DIR_ABSOLUTE}"/final/dune_"${md}"*.whl 2>/dev/null | wc -l)" -gt 1 ]; then
    echo "Error: More than one dune_${md} wheel file found in the final wheelhouse directory." >&2
    exit 1
  fi
done

# default command is "build-wheels.sh"
# this deletes testtols and uggrid source dirs
# forward CI markers so versioneer renders a unique, CI-style wheel version
# (CI -> pep440-pre style, GITHUB_RUN_NUMBER -> unique build number)
DOCKER_RUN="docker run ${DT} --env-file=${ENV_FILE} -e DUNE_SRC_DIR=/home/dxt/src -v ${THISDIR}/../:/home/dxt/src \
  -e LOCAL_USER=${LOCAL_USER} -e LOCAL_GID=${LOCAL_GID} -e LOCAL_UID=${LOCAL_UID} \
  -e WHEELDIR_RELATIVE=${WHEELDIR_RELATIVE} -e PYTHON_VERSION=${PYTHON_VERSION} -e PLATFORM=${PLATFORM} \
  -e CI -e GITHUB_RUN_NUMBER \
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

# ... and that they still run on the oldest CPU in the GitHub-hosted pool.
#
# This wheel is built on one hosted runner and installed by build_docs on another,
# and the pool spans Intel/AMD generations. A dependency whose kernels are pinned
# to the *build* host's ISA then yields a wheel that dies with SIGILL on an older
# runner -- a coin flip, surfacing only as "nbclient DeadKernelError: Kernel died"
# with no cause. openblas without the "dynamic-arch" feature was exactly that; see
# #456 and #467. Grepping the shared objects for AVX-512 cannot replace this:
# with dynamic-arch openblas deliberately *contains* AVX-512 kernels and picks
# among them at runtime, so only executing the code answers the question.
#
# qemu-user fixes what CPUID reports, so a library that dispatches at runtime
# selects baseline kernels and passes, while anything pinned above the baseline
# raises SIGILL right here, on every wheel build, deterministically.
#
# -cpu Nehalem (x86-64-v2) is stricter than the pool actually needs, which costs
# nothing while the wheel is genuinely portable -- numpy/scipy's own manylinux
# wheels dispatch at runtime and pass it. If a third-party wheel ever forces it,
# relax to Haswell (AVX2, no AVX-512), which still catches the class that bit us;
# expect TCG "doesn't support requested feature" warnings for hle/rtm there.
# qemu resolves no PATH, hence the absolute interpreter path.
# ponytail: models CPUID, not the microarchitecture -- catches a statically
# pinned ISA, not an illegal instruction reached only on some data path. Run
# whole notebooks under qemu if that ever turns out to matter.
docker run ${DT} -v "${WHEEL_DIR_ABSOLUTE}"/final:/wheelhouse:ro \
  -v "${THISDIR}/check_baseline_isa.py":/check_baseline_isa.py:ro -i "${TEST_IMAGE}" \
  bash -c "export DEBIAN_FRONTEND=noninteractive \
    && apt-get update -qq && apt-get install -y -qq qemu-user-static \
    && pip install /wheelhouse/dune* \
    && OPENBLAS_NUM_THREADS=1 OMP_NUM_THREADS=1 \
       qemu-x86_64-static -cpu Nehalem \"\$(command -v python)\" /check_baseline_isa.py"

echo '************************************'
echo "Wheels are in ${WHEEL_DIR_ABSOLUTE}/final"
