#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}/.."

git update-index -q --really-refresh && \
    git diff-index --quiet HEAD && \
    git diff --no-ext-diff --quiet --exit-code && \
    git diff --no-ext-diff --quiet

exit $?