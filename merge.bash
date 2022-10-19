#!/bin/bash
#
# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze (2022)
# ~~~

set -xue -o pipefail

err_report() {
  echo "errexit on line $(caller)" >&2
  echo "in dir ${PWD}"
  exit 1
}

trap err_report ERR

THIS_DIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ; pwd -P )"


TARGET_REPO=https://x-auth-token:glpat-yx-kxDpxpGusuJuK6epW@zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-merged
XT=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
GDT=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
SUPER=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt-super
CISHARED=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt-ci
TUTORIALS=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt-tutorials
VCSETUP=https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/vcsetup
TARGET_DIR=${THIS_DIR}/dune-merged
TMP=${THIS_DIR}/tmp
# the subdir where we want to keep the other dune modules
NEW_SUPER=deps
DEFAULT_BRANCH=master

rm -rf ${TMP} ${TARGET_DIR}
mkdir -p ${TMP}
git clone ${XT} ${TARGET_DIR}
cd ${TARGET_DIR}
git submodule update --init --recursive

# VCSETUP
## Merge submodule history+files
git submodule deinit .vcsetup
git rm .vcsetup
git commit -m "[MERGER] remove vcsetup submodule"
rm -rf .git/modules/.vcsetup
git remote add -f VCSETUP ${VCSETUP}
git merge -s ours --no-commit --allow-unrelated-histories VCSETUP/master
git read-tree --prefix=.vcsetup -u VCSETUP/master
git commit -m "[MERGER] merge vcsetup contents"
## Adjust file locations
git rm .pre-commit-config.yaml
git mv .vcsetup/pre-commit-config.yaml .pre-commit-config.yaml
git rm .clang-format
git mv .vcsetup/.clang-format-8 .clang-format
git rm .clang-tidy
git mv .vcsetup/.clang-tidy .clang-tidy
git rm .cmake_format.py
git mv .vcsetup/.cmake_format.py .cmake_format.py
(pre-commit run -a clang-format ; pre-commit run -a cmake-format ) || git commit -am "[MERGER] apply pre-commit"

# CISHARED
## Merge submodule history+files
git submodule deinit .ci/shared
git rm .ci/shared
git commit -m "[MERGER] remove .ci/shared submodule"
rm -rf .git/modules/.ci
git remote add -f CISHARED ${CISHARED}
git merge -s ours --no-commit --allow-unrelated-histories CISHARED/master
git read-tree --prefix=.ci/shared -u CISHARED/master
git rm -f .ci/shared/docker/update_test_dockers.py
git commit -m "[MERGER] merge .ci/shared contents"
## Adjust file locations
git rm .binder
git mv .ci/shared/binder .binder

# SUPER
## Merge submodule history+files
git remote add -f SUPER ${SUPER}
git read-tree --prefix=${NEW_SUPER} -u SUPER/consolidation
git commit -m "[MERGER] merge gdt-super contents"
##
git rm .gitmodules
git mv ${NEW_SUPER}/.gitmodules .
sed -i "s;path = ;path = ${NEW_SUPER}/;g" .gitmodules
git submodule update --init --recursive
git commit -am "[MERGER] setup dependency submodules"
## remove xt+gdt
pushd ${NEW_SUPER}
for i in dune-xt dune-gdt .ci/shared bin; do
    git submodule deinit ${i}
    git rm ./${i}
    rm -rf ../.git/modules/${i}
done
git rm .travis.yml LICENSE README.md set_xt_submodules_to_ssh.bash xt_to_master.bash
git mv make_docker_wheels.bash ../
git commit -m "[MERGER] cleanup unnecessary deps+files"
popd

# dune-gdt
## remove files that would override changed files in merge
git clone ${GDT} ${TMP}/gdt
cat .mailmap ${TARGET_DIR}/.mailmap | grep -v "#" | sort | uniq > mailmap
mv mailmap ${TARGET_DIR}/.mailmap
git commit ${TARGET_DIR}/.mailmap -m "[MERGER] merge mailmap"

pushd ${TMP}/gdt
git rm -rf .drone.yml .clang* .cmake* .pre* .ci python/dune/__init__.py python/test/base.py \
    python/LICENSE.txt python/.gitignore .vcsetup LICENSE .mailmap
git commit -am "[MERGER] prepare gdt for merge"
popd

# Other infra
cp ${THIS_DIR}/${BASH_SOURCE[0]} ${TARGET_DIR}
cp ${THIS_DIR}/Dockerfile ${THIS_DIR}/entrypoint.bash ${TARGET_DIR}/${NEW_SUPER}
git add merge.bash ${NEW_SUPER}/{Dockerfile,entrypoint.bash}
git commit -m "[MERGER] container/merge infrastructure"
git remote add TARGET_REPO ${TARGET_REPO}
git push -f TARGET_REPO master

# tutorials
git clone ${TUTORIALS} ${TMP}/TUTORIALS
pushd ${TMP}/TUTORIALS
git subtree split -P tutorials -b split
popd
git remote add -f TUTORIALS ${TMP}/TUTORIALS
git merge -s ours --no-commit --allow-unrelated-histories TUTORIALS/split
git read-tree --prefix=tutorials -u TUTORIALS/split
git commit -m "[MERGER] merge tutorials contents"
git push -f TARGET_REPO master

## load conflict resolutions
### sadly this doesn;t actually work atm
git remote add -f GDT ${TMP}/gdt
rm -rf .git/rr-cache/*
git checkout rr-cache
mkdir -p .git/rr-cache/
cp -ra * .git/rr-cache/
git checkout ${DEFAULT_BRANCH}
## do the actual merge
git merge --rerere-autoupdate --allow-unrelated-histories GDT/master


# final push
git diff --exit-code
git push TARGET_REPO master

# dunecontrol finds `dune.module` files in here
rm -rf .git/rr-cache

IMAGE=${GITLAB_REGISTRY_IMAGE:-dunecommunity/dune-merged}
docker build -t ${IMAGE}:${CI_COMMIT_SHA} -f ${NEW_SUPER}/Dockerfile ${NEW_SUPER}
docker push ${IMAGE}:${CI_COMMIT_SHA}
docker run -it -v $PWD:/src ${IMAGE}:${CI_COMMIT_SHA} dunecontrol --opts=deps/config.opts/clang-debug all
