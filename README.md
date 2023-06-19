# dune-gdt

dune-gdt is a [DUNE](http://www.dune-project.org/) module which provides a generic
discretization toolbox for grid-based numerical methods. It contains building blocks - like
local operators, local evaluations, local assemblers - for discretization methods and suitable
discrete function spaces.

[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![pipeline status](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/badges/master/pipeline.svg)](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/-/commits/master)

## Getting started

### using the pre-configured development docker environment for C++ development

We provide the `ghcr.io/dune-gdt/dune-gdt/dev` docker image with all required dependencies installed, along with the `./docker/run_dev.sh` script to easy daily work.
First clone the repository and make all submodules available:
```bash
git clone https://github.com/dune-gdt/dune-gdt.git
cd dune-gdt
git submodule update --init --recursive
```
Then execute the script to obtain an interactive shell with a persistent home and the checked out directory mounted as `~/dune-gdt`:
```bash
./docker/run_dev.sh
```
This will download the `ghcr.io/dune-gdt/dune-gdt/dev` docker image, which may optionally be built locally beforehand by executing `./docker/build_dev.sh`.

The basic steps for a first setup to work on dune-gdt within the development container are as follows:

1. choose one of `deps/config.opts/` to select the compiler and build type
2. create and use a Python virtualenv
2. use `dunecontrol` to configure and build all dune modules
4. prescribe variable overrides (say for non-standard compiler names like `CC=gcc-10 CXX=g++-10 F77=gfortran-10`) on the command line

For example, the following set of commands will build everything for a _Debug_ build with _gcc_ the _Ninja_ generator:

```bash
export OPTS=gcc-debug.ninja
virtualenv -p python3.9 venvs/venv-${OPTS}
. venvs/venv-${OPTS}/bin/activate

# configure all dune modules to compile with warnings not as errors ...
WERROR=0 ./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} configure
# but configure dune-gdt to compile with warnings as errors (does not compile with pybind11 :/)
# WERROR=1 ./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} --only=dune-gdt configure

# build all libraries
./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} all

# optionally: build and install the Python bindings (and prevent make from freezing the system)
if [[ "${OPTS: -6}" == ".ninja" ]] ; then export MAKE=ninja ; else export MAKE="make -j(($(nproc) - 1))"; fi
./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} --only=dune-gdt bexec "$MAKE bindings_no_ext"
./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} --only=dune-gdt bexec "$MAKE install_python"

# optionally: build all tests
./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${OPTS} --builddir=${PWD}/build/${OPTS} --only=dune-gdt bexec "$MAKE test_binaries"
```

## License

dune-gdt is dual licensed as [BSD 2](http://opensource.org/licenses/BSD-2-Clause) or ([GPL-2.0+](http://opensource.org/licenses/gpl-license) with [runtime exception](https://dune-project.org/about/license/)), see [LICENSE](LICENSE) for details.


## Citing

dune-gdt also contains dune-xt, which is an eXTensions module for [DUNE](https://www.dune-project.org).
There is a paper describing some of the concepts behind dune-xt.
While already dated (in particular, the four modules dune-xt-common, dune-xt-grid, dune-xt-la and dune-xt-functions have been merged into the single dune-gdt module), most ideas still apply:

    T. Leibner and R. Milk and F. Schindler: "Extending DUNE: The dune-xt modules"
    Archive of Numerical Software, 5:193-216, 2017
    https://www.doi.org/10.11588/ans.2017.1.27720
