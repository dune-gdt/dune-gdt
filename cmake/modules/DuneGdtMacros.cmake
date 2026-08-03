# ~~~
# This file is part of the dune-gdt project:
# https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
# or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
# with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
# Felix Schindler (2014, 2016 - 2018)
# René Fritze     (2017 - 2018)
# ~~~

set(DUNE_GRID_EXPERIMENTAL_GRID_EXTENSIONS TRUE)

include(XtCompilerSupport)
include(XtTooling)
include(DuneXTHints)

set(DXT_DONT_LINK_PYTHON_LIB
    ${DXT_DONT_LINK_PYTHON_LIB}
    CACHE STRING "wheelbuilders want to set this to 1")

# library checks  #########################################################################
find_package(PkgConfig)

find_package(
  Boost 1.48.0 REQUIRED
  COMPONENTS atomic
             chrono
             date_time
             filesystem
             system
             thread
             timer)
dune_register_package_flags(INCLUDE_DIRS ${Boost_INCLUDE_DIRS})

if(TARGET Boost::headers)
  dune_register_package_flags(LIBRARIES Boost::headers)
endif()

foreach(
  boost_lib IN
  ITEMS atomic
        chrono
        date_time
        filesystem
        system
        thread
        timer)
  if(TARGET Boost::${boost_lib})
    dune_register_package_flags(LIBRARIES Boost::${boost_lib})
  else()
    string(TOUPPER "${boost_lib}" _boost_lib)
    dune_register_package_flags(LIBRARIES ${Boost_${_boost_lib}_LIBRARY})
  endif()
endforeach()

find_package(Eigen3 3.4.0 REQUIRED)
dune_register_package_flags(INCLUDE_DIRS ${EIGEN3_INCLUDE_DIR})

find_package(MKL)

if(MKL_FOUND)
  set(HAVE_LAPACKE 0)
else(MKL_FOUND)
  find_package(LAPACKE)
endif(MKL_FOUND)

# A LAPACKE implementation (either standalone or the one inside the Intel MKL) is a hard requirement: it is the only
# backend for the generalized eigenvalue problem (dune/xt/la/generalized-eigen-solver/default.hh) and the fast path of
# the eigen solver, the matrix inverter, the QR and the Cholesky algorithms. Without it Dune::XT::Common::Lapacke
# reports available() == false at *compile* time, so all of that silently degrades or throws at runtime and the
# corresponding tests compile down to empty stubs -- which, before this became mandatory, is exactly what happened in CI
# without anyone noticing.
if(NOT HAVE_MKL AND NOT HAVE_LAPACKE)
  set(_lapacke_hint
      "Install a LAPACKE development package (liblapacke-dev on Debian/Ubuntu, lapack-devel on"
      " Fedora/RHEL) or the Intel MKL. vcpkg builds already get it from the lapack-reference overlay"
      " port in .vcpkg-overlays/ports/.\nNote that LAPACKE_LIBRARY and LAPACKE_INCLUDE_DIRS are cached"
      " variables, so a build directory configured before LAPACKE was available keeps its stale"
      " -NOTFOUND and never re-searches: unset those two (or delete CMakeCache.txt) before retrying.")
  string(REPLACE ";" "" _lapacke_hint "${_lapacke_hint}")
  if(LAPACKE_NOT_FOUND_REASONS)
    string(REPLACE ";" "\n  - " _lapacke_reasons ";${LAPACKE_NOT_FOUND_REASONS}")
    message(FATAL_ERROR "Neither the Intel MKL nor LAPACKE was found, but one of them is required.${_lapacke_reasons}"
                        "\n${_lapacke_hint}")
  else()
    message(FATAL_ERROR "Neither the Intel MKL nor LAPACKE was found, but one of them is required.\n${_lapacke_hint}")
  endif()
endif()

find_package(Spe10Data)

# intel mic and likwid don't mix
if(NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "k1om")
  include(FindLIKWID)
  find_package(LIKWID)

  if(LIKWID_FOUND)
    dune_register_package_flags(INCLUDE_DIRS ${LIKWID_INCLUDE_DIR} LIBRARIES ${LIKWID_LIBRARY})
  endif(LIKWID_FOUND)
else()
  set(HAVE_LIKWID 0)
endif()

include(DuneTBB)

if(HAVE_MPI)
  include(FindMPI4PY)

  if(MPI4PY_FOUND)

  else()
    execute_process(
      COMMAND ${RUN_IN_ENV_SCRIPT} pip install mpi4py
      ERROR_VARIABLE shell_error
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    myfind_mpi4py()
  endif()

  if(MPI4PY_FOUND)
    # this only works in dependent modules
    dune_register_package_flags(INCLUDE_DIRS "${MPI4PY_INCLUDE_DIR}")

    # this only works in dune-gdt itself
    include_directories(SYSTEM "${MPI4PY_INCLUDE_DIR}")
  else()
    message(FATAL_ERROR "mpi4py is required, but could not be installed")
  endif()
endif()

# dune-common's FindX does not set this correctly
include_directories(SYSTEM ${PYTHON_INCLUDE_DIRS})

# end library checks  #####################################################################

# misc vars  #########################################################################
set(CMAKE_EXPORT_COMPILE_COMMANDS "ON")
set(DS_MAX_MIC_THREADS
    120
    CACHE STRING "")
set(DUNE_XT_COMMON_TEST_DIR ${dune-gdt_SOURCE_DIR}/dune/xt/common/test)
set(ENABLE_PERFMON
    0
    CACHE STRING "enable likwid performance monitoring API usage")

if(NOT DS_HEADERCHECK_DISABLE)
  set(ENABLE_HEADERCHECK 1)
endif(NOT DS_HEADERCHECK_DISABLE)

set(DXT_TEST_TIMEOUT
    1000
    CACHE STRING "per-test timeout in seconds")
set(DXT_TEST_PROCS
    3
    CACHE STRING "run N tests in parallel")

set(DUNE_GRID_EXPERIMENTAL_GRID_EXTENSIONS TRUE)

include(DunePybindxiUtils)

# TODO there's no real way to require packages present at configure time? - jinja2,pyparsing is needed for test
# templating can't use the run-in-env script here, because it will try to use dune.common which we're trying to install
dune_execute_process(
  COMMAND
  ${CMAKE_BINARY_DIR}/dune-env/bin/python
  -m
  pip
  install
  jinja2
  pyparsing)
# Install the dune wheels from the vcpkg wheelhouse into the virtualenv. This file runs twice during configure: once
# when included from the top-level CMakeLists.txt — possibly before dune-common's cmake has created the virtualenv, in
# which case the installs are skipped — and once as dune-gdt's module-test macros after the virtualenv exists, which is
# the pass that actually installs into a fresh venv. The wheel filenames are globbed (can't use dune-common_WHEELHOUSE
# here, because it is not set yet) so a vcpkg port bump cannot silently leave this pointing at a wheel that no longer
# exists. The installs run strictly sequentially, one execute_process each: passing several COMMANDs to a single
# execute_process runs them as a concurrent pipeline, and two pip processes racing in the same virtualenv corrupt
# site-packages (half-uninstalled "~une-common" leftovers, BrokenPipeError). The dune-common version is pinned by
# installing the exact wheelhouse wheel by path before anything depends on it: pip then never resolves "dune-common" as
# an abstract requirement (and won't upgrade an already-satisfied one), so it cannot pull a mismatching release from
# PyPI. --no-index would pin even harder but breaks the install: the wheels' third-party dependencies (portalocker,
# numpy, ...) are not in the wheelhouse and must come from PyPI.
if(EXISTS ${CMAKE_BINARY_DIR}/dune-env/bin/python)
  if(NOT VCPKG_TARGET_TRIPLET)
    set(VCPKG_TARGET_TRIPLET x64-linux)
  endif()
  set(_dune_wheelhouse ${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/share/dune/wheelhouse)
  foreach(dune_wheel_pkg dune_common dune_testtools)
    file(GLOB _dune_wheel ${_dune_wheelhouse}/${dune_wheel_pkg}-*.whl)
    list(LENGTH _dune_wheel _dune_wheel_count)
    if(NOT _dune_wheel_count EQUAL 1)
      message(
        FATAL_ERROR "Expected exactly one ${dune_wheel_pkg} wheel in ${_dune_wheelhouse}, found: '${_dune_wheel}'")
    endif()
    execute_process(
      COMMAND ${CMAKE_BINARY_DIR}/dune-env/bin/python -m pip install --find-links=${_dune_wheelhouse} ${_dune_wheel}
      RESULT_VARIABLE _pip_install_result
      OUTPUT_VARIABLE pip_install_log ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
      ERROR_VARIABLE pip_install_log
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(NOT _pip_install_result EQUAL 0)
      message(FATAL_ERROR "Installing ${_dune_wheel} into the virtualenv failed:\n${pip_install_log}")
    endif()
  endforeach()
  unset(_dune_wheelhouse)
  unset(_dune_wheel)
  unset(_dune_wheel_count)
  unset(_pip_install_result)
else()
  message(STATUS "Skipping dune wheel installs, the virtualenv does not exist yet")
endif()
