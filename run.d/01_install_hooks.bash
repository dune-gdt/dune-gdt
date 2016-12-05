#!/bin/bash

set -e

SRC_HOOKDIR="$(cd "$(dirname ${BASH_SOURCE[0]})/../hooks" ;  pwd -P )"
# this _might_ fail with git >= 2.9 if core.hooksPath is set
DST_HOOKDIR="$(git rev-parse --git-dir)/hooks"

cp -a ${SRC_HOOKDIR}/* ${DST_HOOKDIR}
