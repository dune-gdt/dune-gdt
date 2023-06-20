#!/bin/bash

# the dir containing this script is the base
BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}"

# ensure there are no open changes, as the commit hash will enter the docker image
./check_for_clean_working_dir.sh
if [[ $? != 0 ]] ; then
    >&2 echo "ERROR: refusing to build docker image in dirty working directory!"
    exit 1
fi

echo -n "computing hashes of current state ... "
export DATE=$(date -u -Iseconds)
export DEV_HASH=$(./compute_dev_hash.sh)
export HASH=$(./compute_ci_hash.sh)
export VCS_REF=$(git log -1 --pretty=format:"%H")
echo "done"

echo -n "patching docker/ci/Dockerfile ... "
sed -i "s/FROM_DEV_HASH/${FROM_DEV_HASH}/" ci/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi
sed -i "s/LABEL_BUILD_DATE/${DATE}/" ci/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi
sed -i "s/LABEL_VCS_REF/${VCS_REF}/" ci/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi

echo "done"
echo "building ghcr.io/dune-gdt/dune-gdt/ci:${HASH}"
echo "======================================================================================================="

docker build -t ghcr.io/dune-gdt/dune-gdt/ci:${HASH} -f ci/Dockerfile ci/

if [[ $? != 0 ]] ; then
    >&2 echo "======================================================================================================="
    >&2 echo "... failed (see above)!"
    >&2 echo "restoring docker/ci/Dockerfile ..."
    git restore ci/Dockerfile
    exit 1
fi

echo "======================================================================================================="
echo "... done!"
echo -n "restoring docker/ci/Dockerfile ... "
git restore ci/Dockerfile
if [[ $? != 0 ]] ; then
    >&2 echo "... failed (see above)!"
    exit 1
fi
echo "done"
echo "======================================================================================================="
echo "Execute"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/ci:${HASH}"
echo "to push the image and"
echo "    docker tag ghcr.io/dune-gdt/dune-gdt/ci:${HASH} ghcr.io/dune-gdt/dune-gdt/ci:latest"
echo "    docker push ghcr.io/dune-gdt/dune-gdt/ci:latest"
echo "to overwrite the latest tag."
echo "======================================================================================================="
