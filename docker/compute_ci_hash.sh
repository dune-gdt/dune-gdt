#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}"

export DEV_HASH=$(./compute_dev_hash.sh)
export CI_HASH=$(sha256sum ci/* compute_ci_hash.sh | sort | sha256sum | cut -d ' ' -f 1)
export SUBMODULES_HASH=$(git submodule status | sort | sha256sum | sha256sum | cut -d ' ' -f 1)
export OPTS_HASH=$(sha256sum ../deps/config.opts/* | sort | sha256sum | cut -d ' ' -f 1)
echo $(echo $DEV_HASH $CI_HASH $SUBMODULES_HASH $OPTS_HASH | sort | sha256sum | cut -d ' ' -f 1)