#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}"

export DEV_HASH=$(./compute_dev_hash.sh)
export CI_HASH=$(sha256sum ci/* compute_ci_hash.sh | sort | sha256sum | cut -d ' ' -f 1)
# Any changes in these require an update of DUNE_GDT_REF in docker/ci/Dockerfile
export SUBMODULES_HASH=$(git ls-tree HEAD deps/ | grep commit | sha256sum | sort | sha256sum | cut -d ' ' -f 1)
export OPTS_HASH=$(sha256sum ../deps/config.opts/* | sort | sha256sum | cut -d ' ' -f 1)
echo $(echo $DEV_HASH $CI_HASH $SUBMODULES_HASH $OPTS_HASH | sort | sha256sum | cut -d ' ' -f 1)
