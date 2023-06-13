dune-gdt
========

dune-gdt is a [DUNE](http://www.dune-project.org/) module which provides a generic
discretization toolbox for grid-based numerical methods. It contains building blocks - like
local operators, local evaluations, local assemblers - for discretization methods and suitable
discrete function spaces.

[![pre-commit](https://img.shields.io/badge/pre--commit-enabled-brightgreen?logo=pre-commit&logoColor=white)](https://github.com/pre-commit/pre-commit)
[![pipeline status](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/badges/master/pipeline.svg)](https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt/-/commits/master)


License
-------

dune-gdt is dual licensed as [BSD 2](http://opensource.org/licenses/BSD-2-Clause) or ([GPL-2.0+](http://opensource.org/licenses/gpl-license) with [runtime exception](https://dune-project.org/about/license/)), see [LICENSE](LICENSE) for details.


Citing
------

dune-gdt also contains dune-xt, which is an eXTensions module for [DUNE](https://www.dune-project.org).
There is a paper describing some of the concepts behind dune-xt.
While already dated (in particular, the four modules dune-xt-common, dune-xt-grid, dune-xt-la and dune-xt-functions have been merged into the single dune-gdt module), most ideas still apply:

    T. Leibner and R. Milk and F. Schindler: "Extending DUNE: The dune-xt modules"
    Archive of Numerical Software, 5:193-216, 2017
    https://www.doi.org/10.11588/ans.2017.1.27720


Getting started with native development
---------------------------------------

The basic steps are to

1. Ensure all system requirements are installed.
   This varies from system to system, a good starting point is the [Dockerfile used in CI](https://github.com/dune-gdt/dune-merged/blob/main/deps/Dockerfile).
   We at least require a decent compiler (gcc or clang) and cmake.
2. Check out the code. This usually amounts to something like
   ```bash
   git clone https://github.com/dune-gdt/dune-merged.git dune-gdt
   cd dune-gdt
   git submodule update --init --recursive
   ```
3. Create a Python virtualenv (using a supported Python version, check for `python` in the respective [workflow](https://github.com/dune-gdt/dune-merged/blob/main/.github/workflows/wheels.yml)):
   ```bash
   python3.9 -m venv venv
   . venv/bin/activate
   ```
4. Select a configation to work with by means of a config file like the ones in [deps/config.opts](https://github.com/dune-gdt/dune-merged/tree/main/deps/config.opts):
   ```bash
   export CONFIG_OPTS=clang-relwithdebinfo.ninja
   ```
5. (Optionally) onfigure all modules to check for missing dependencies:
   ```bash
   ./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${CONFIG_OPTS} --builddir=./deps/build-${CONFIG_OPTS} configure
   ```
6. Build all modules:
   ```bash
   ./deps/dune-common/bin/dunecontrol --opts=./deps/config.opts/${CONFIG_OPTS} --builddir=./deps/build-${CONFIG_OPTS} all
   ```


Interactive development of Python bindings
------------------------------------------

We recommend running the preconfigured docker environment in a [rootless docker setup](https://docs.docker.com/engine/security/rootless/).
After checking out the repository

    git clone https://github.com/dune-gdt/dune-gdt.git
    cd dune-gdt

simply build the docker image

    ./docker/build_debian-qtcreator.sh

and execute

    ./docker/run_debian-qtcreator.sh

This will mount the working directory as `/root/dune-gdt` and run a `bash` within the docker container as user `root` (note that due to the rootless docker setup, running as root within the container is equivalent to running as your usual user outside the container; in particular, all files created within the `/root/dune-gdt` folder are persistent and have the correct permissions).
To configure and compile the bindings, enter the mounted directory and call the initial setup script

    cd dune-gdt  # /root/dune-gdt
    ./build_interactive_bindings_venv.sh

This will take a considerable amount of time and resources. Afterwards, follow the printed instructions to obtain a virtualenv where the bindings are available, i.e., running

    OPTS=clang-relwithdebinfo.ninja source deps/environments/debian-full/PATH.sh
    ipython

will leave you with an iPython prompt which will allow you to

    import dune.xt
    import dune.gdt
