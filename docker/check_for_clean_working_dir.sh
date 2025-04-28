#!/bin/bash

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" || exit ;  pwd -P)"
cd "${BASEDIR}/.." || exit

git update-index -q --really-refresh && \
    git diff-index --quiet HEAD && \
    git diff --no-ext-diff --quiet --exit-code && \
    git diff --no-ext-diff --quiet

exit $?