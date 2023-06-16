#!/bin/bash

# the dir containing this script is the base
BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"

cd "${BASEDIR}"
export HASH=$(sha256sum dev/* | sha256sum | cut -d ' ' -f 1)
echo "======================================================================================================="
echo "Building ghcr.io/dune-gdt/dune-gdt/dev:${HASH}"
echo "======================================================================================================="

docker build -t ghcr.io/dune-gdt/dune-gdt/dev:${HASH} -f dev/Dockerfile dev/

if [[ $? != 0 ]] ; then
    >&2 echo "======================================================================================================="
    >&2 echo "... failed (see above)!"
    >&2 echo "======================================================================================================="
    exit 1
fi

echo "======================================================================================================="
echo "... done!"
echo "======================================================================================================="
echo "Execute"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/dev:${HASH}"
echo "to push the image and"
echo "    docker tag ghcr.io/dune-gdt/dune-gdt/dev:${HASH} ghcr.io/dune-gdt/dune-gdt/dev:latest"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/dev:latest"
echo "to overwrite the latest tag."
echo "======================================================================================================="
echo "Updating docker/ci/Dockerfile ..."

sed -i \
    "s;^FROM ghcr.io/dune-gdt/dune-gdt/dev:.*$;FROM ghcr.io/dune-gdt/dune-gdt/dev:${HASH};" \
    "${BASEDIR}/ci/Dockerfile"

if [[ $? != 0 ]] ; then
    >&2 echo "======================================================================================================="
    >&2 echo "... failed (see above)!"
    >&2 echo "======================================================================================================="
    exit 1
fi

echo "... done!"
echo "Consider to run ./docker/build_ci.sh next!"
echo "======================================================================================================="
