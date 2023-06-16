#!/bin/bash

# the dir containing this script is the base
BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"

cd "${BASEDIR}"
export DEV_HASH=$(sha256sum dev/* | sha256sum | cut -d ' ' -f 1)
export CI_HASH=$(sha256sum ci/* | sha256sum | cut -d ' ' -f 1)
export SUBMODULES_HASH=$(git submodule status | sha256sum | sha256sum | cut -d ' ' -f 1)
export OPTS_HASH=$(sha256sum ../deps/config.opts/* | sha256sum | cut -d ' ' -f 1)
export HASH=$(echo $DEV_HASH $CI_HASH $SUBMODULES_HASH $OPTS_HASH | sha256sum | sha256sum | cut -d ' ' -f 1)
echo "======================================================================================================="
echo "Building ghcr.io/dune-gdt/dune-gdt/ci:${HASH}"
echo "======================================================================================================="

docker build -t ghcr.io/dune-gdt/dune-gdt/ci:${HASH} -f ci/Dockerfile ci/

if [[ $? -eq 0 ]] ; then

    echo "======================================================================================================="
    echo "... done!"
    echo "======================================================================================================="
    echo "Execute"
    echo "    docker push ghcr.io/dune-gdt/dune-gdt/ci:${HASH}"
    echo "to push the image and"
    echo "    docker tag ghcr.io/dune-gdt/dune-gdt/ci:${HASH} ghcr.io/dune-gdt/dune-gdt/ci:latest"
    echo "    docker push ghcr.io/dune-gdt/dune-gdt/ci:latest"
    echo "to overwrite the latest tag."

else
    >&2 echo "======================================================================================================="
    >&2 echo "... failed (see above)!"
    >&2 echo "======================================================================================================="
    exit 1
fi
