#!/bin/bash

BASEDIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ;  pwd -P)"

echo $(sha256sum dev/* | sha256sum | cut -d ' ' -f 1)