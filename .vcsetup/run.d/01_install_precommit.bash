#!/bin/bash

set -e

THIS_DIR="$(cd "$(dirname ${BASH_SOURCE[0]:-$0})" ; pwd -P )"
cd ${THIS_DIR}/../.. && pre-commit install
