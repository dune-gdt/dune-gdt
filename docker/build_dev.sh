#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}"

# ensure there are no open changes, as the commit hash will enter the docker image
./check_for_clean_working_dir.sh
if [[ $? != 0 ]] ; then
    >&2 echo "ERROR: refusing to build docker image in dirty working directory!"
    exit 1
fi

echo -n "computing hashes of current state ... "
export HASH=$(./compute_dev_hash.sh)
export DATE=$(date -u -Iseconds)
export VCS_REF=$(git log -1 --pretty=format:"%H")
echo "done"

echo -n "patching docker/ci/Dockerfile ... "
sed -i "s/LABEL_BUILD_DATE/${DATE}/" dev/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi
sed -i "s/LABEL_VCS_REF/${VCS_REF}/" dev/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi

echo "done"
echo "building ghcr.io/dune-gdt/dune-gdt/dev:${HASH}"
echo "======================================================================================================="

docker build -t ghcr.io/dune-gdt/dune-gdt/dev:${HASH} -f dev/Dockerfile dev/

if [[ $? != 0 ]] ; then
    >&2 echo "======================================================================================================="
    >&2 echo "... failed (see above)!"
    >&2 echo "restoring docker/dev/Dockerfile ..."
    git restore dev/Dockerfile
    exit 1
fi

echo "======================================================================================================="
echo "... done!"
echo -n "restoring docker/dev/Dockerfile ... "
git restore dev/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi
echo "done"
echo "======================================================================================================="
echo "Execute"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/dev:${HASH}"
echo "to push the image and"
echo "    docker tag ghcr.io/dune-gdt/dune-gdt/dev:${HASH} ghcr.io/dune-gdt/dune-gdt/dev:latest"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/dev:latest"
echo "to overwrite the latest tag."
echo "======================================================================================================="
echo "Consider to run ./docker/build_ci.sh next!"
echo "======================================================================================================="
