#!/bin/bash

BASEDIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" || exit ;  pwd -P)"
cd "${BASEDIR}" || exit

echo "$(sha256sum dev/* compute_dev_hash.sh | sort | sha256sum | cut -d ' ' -f 1)"
