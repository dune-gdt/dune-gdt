#!/bin/bash

set -e

if [ "X${OPTS}" == "X" ]; then
    echo No OPTS specified, defaulting to clang-relwithdebinfo.ninja!
    export OPTS=clang-relwithdebinfo.ninja
fi
if [ "X${PYTHON}" == "X" ]; then
    echo No PYTHON specified, defaulting to python3!
    export PYTHON=python3
fi

# define environment in case we are not in one of our dockers
# determines which one to use below environments/
if [ "X${DXT_ENVIRONMENT}" == "X" ]; then
    echo No environment specified, defaulting to debian-full!
    export DXT_ENVIRONMENT=debian-full
fi

# initialize the virtualenv, if not yet present
export BASEDIR="${PWD}"
mkdir -p deps/environments/${DXT_ENVIRONMENT}/venv
if [ -e deps/environments/${DXT_ENVIRONMENT}/venv/${OPTS} ]; then
  echo not creating virtualenv in deps/environments/${DXT_ENVIRONMENT}/venv/${OPTS}, already exists
else
  cd deps/environments/${DXT_ENVIRONMENT}/venv
  virtualenv --python=${PYTHON} ${OPTS}
  source ${OPTS}/bin/activate
  deactivate
fi
cd "${BASEDIR}"
unset BASEDIR

# load the variables of this environment, sources the virtualenv
VENV=deps/environments/${DXT_ENVIRONMENT}/venv/${OPTS} source deps/environments/${DXT_ENVIRONMENT}/PATH.sh

# build dune
if [ "${OPTS: -6}" == ".ninja" ]; then
  MAKE=ninja
else
  MAKE="make -j(($(nproc) - 1))"
fi

cd "${BASEDIR}"
# configure
WERROR=0 ./deps/dune-common/bin/dunecontrol --opts=deps/config.opts/$OPTS --builddir=${BASEDIR}/build-$OPTS configure
WERROR=1 ./deps/dune-common/bin/dunecontrol --opts=deps/config.opts/$OPTS --only=dune-gdt --builddir=${BASEDIR}/build-$OPTS configure
# make all
./deps/dune-common/bin/dunecontrol --builddir=${BASEDIR}/build-$OPTS bexec "$MAKE all"
# make bindings
./deps/dune-common/bin/dunecontrol --builddir=${BASEDIR}/build-$OPTS --only=dune-gdt bexec "$MAKE bindings_no_ext"
./deps/dune-common/bin/dunecontrol --builddir=${BASEDIR}/build-$OPTS --only=dune-gdt bexec "$MAKE install_python"

cd "${BASEDIR}"
echo
echo "All done! From now on run"
echo "  OPTS=$OPTS source deps/environments/${DXT_ENVIRONMENT}/PATH.sh"
echo "to activate the virtualenv!"
