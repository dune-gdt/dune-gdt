#!/bin/bash

COV_OPTION="--cov=dune"

THIS_DIR="$(cd "$(dirname ${BASH_SOURCE[0]})" ; pwd -P )"
BINARY_DIR="${1}"

for fn in ${THIS_DIR}/../../tutorials/source/example__*md ; do
  mystnb-to-jupyter -o "${fn}" "${fn/example/..\/converted_example}".ipynb
done

COMMON_PYTEST_OPTS="--junitxml=${BINARY_DIR}/dune/pytest_results_tutorials.xml \
  --cov-report= ${COV_OPTION} --cov-config=${BINARY_DIR}/python/xt/setup.cfg --cov-context=test"
export COVERAGE_FILE=${BINARY_DIR}/coverage-tutorials
# manually add plugins to load that are excluded for other runs
cd ${THIS_DIR}/../..
xvfb-run -a pytest ${COMMON_PYTEST_OPTS} --nb-coverage -s -p no:pycharm -v -s\
  -p nb_regression -p notebook tutorials/test_tutorials.py

