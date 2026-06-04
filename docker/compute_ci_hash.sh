#!/bin/bash

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" || exit ;  pwd -P)"
cd "${BASEDIR}" || exit

DEV_HASH=$(./compute_dev_hash.sh); export DEV_HASH
CI_HASH=$(sha256sum ci/* compute_ci_hash.sh | sort | sha256sum | cut -d ' ' -f 1); export CI_HASH
# Any changes in these require an update of DUNE_GDT_REF in docker/ci/Dockerfile
SUBMODULES_HASH=$(git ls-tree HEAD deps/ | grep commit | sha256sum | sort | sha256sum | cut -d ' ' -f 1); export SUBMODULES_HASH
OPTS_HASH=$(sha256sum ../deps/config.opts/* | sort | sha256sum | cut -d ' ' -f 1); export OPTS_HASH
echo "$DEV_HASH" "$CI_HASH" "$SUBMODULES_HASH" "$OPTS_HASH" | sort | sha256sum | cut -d ' ' -f 1
