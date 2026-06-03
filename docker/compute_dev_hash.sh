#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"
cd "${BASEDIR}"

echo $(sha256sum dev/* compute_dev_hash.sh | sort | sha256sum | cut -d ' ' -f 1)
