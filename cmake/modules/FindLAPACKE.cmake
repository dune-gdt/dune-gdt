# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2017)
#   René Fritze     (2018 - 2019, 2026)
#   Tobias Leibner  (2017 - 2018, 2020)
# ~~~

include(DuneXTHints)

# Locating LAPACKE means finding three independent things: the library, the lapacke.h header, and a usable LAPACK (the
# LAPACKE_* entry points are thin C wrappers around LAPACK's Fortran symbols). Each is recorded separately below, so
# that a partial installation produces an actionable message rather than a bare "not found" -- see the mandatory check
# in DuneGdtMacros.cmake, which turns any of these into a configure error.
set(LAPACKE_NOT_FOUND_REASONS "")

find_package(BLAS)
find_package(LAPACK)

message("-- checking for lapacke library")
# PATH_SUFFIXES rather than only HINTS: vcpkg (and some distributions) keep LAPACKE next to its BLAS provider, e.g. in
# <prefix>/lib/openblas or <prefix>/lib/lapack, and those prefixes reach us through CMAKE_PREFIX_PATH rather than
# through the /usr-and-friends LIB_HINTS from DuneXTHints.
find_library(
  LAPACKE_LIBRARY
  NAMES lapacke liblapacke
  HINTS ${LIB_HINTS}
  PATH_SUFFIXES lapacke lapack openblas)
if("${LAPACKE_LIBRARY}" MATCHES "LAPACKE_LIBRARY-NOTFOUND")
  message("--   library 'LAPACKE' not found")
  list(APPEND LAPACKE_NOT_FOUND_REASONS "no LAPACKE library found (looked for 'lapacke')")
else()
  message("--   found LAPACKE library: ${LAPACKE_LIBRARY}")
  set(LAPACKE_LIBRARIES "${LAPACKE_LIBRARY}")
endif()

message("-- checking for lapacke.h header")
set(LAPACKE_HEADER_INCLUDE_HINTS "")
append_to_each("${INCLUDE_HINTS}" "lapacke/" LAPACKE_HEADER_INCLUDE_HINTS)
append_to_each("${INCLUDE_HINTS}" "openblas/" LAPACKE_HEADER_INCLUDE_HINTS)
list(APPEND LAPACKE_HEADER_INCLUDE_HINTS ${INCLUDE_HINTS})
find_path(
  LAPACKE_INCLUDE_DIRS lapacke.h
  HINTS ${LAPACKE_HEADER_INCLUDE_HINTS}
  PATH_SUFFIXES lapacke openblas)
if("${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")
  message("--   lapacke.h header not found")
  list(APPEND LAPACKE_NOT_FOUND_REASONS "no lapacke.h header found")
else()
  message("--   found lapacke.h header: ${LAPACKE_INCLUDE_DIRS}")
  include_sys_dir("${LAPACKE_INCLUDE_DIRS}")
endif()

if(NOT LAPACK_FOUND)
  list(APPEND LAPACKE_NOT_FOUND_REASONS
       "find_package(LAPACK) failed, so the Fortran symbols behind the LAPACKE wrappers cannot resolve")
endif()

if(BLAS_FOUND)
  list(APPEND LAPACKE_LIBRARIES ${BLAS_LIBRARIES})
  list(APPEND LAPACKE_LIBRARIES gfortran)
endif()
if(LAPACK_FOUND)
  list(APPEND LAPACKE_LIBRARIES ${LAPACK_LIBRARIES})
endif()

list(LENGTH LAPACKE_NOT_FOUND_REASONS _lapacke_missing_count)
if(_lapacke_missing_count EQUAL 0)
  set(LAPACKE_FOUND 1)
else()
  set(LAPACKE_FOUND 0)
endif()
unset(_lapacke_missing_count)

set(HAVE_LAPACKE ${LAPACKE_FOUND})

# register all LAPACKE related flags
if(LAPACKE_FOUND)
  dune_register_package_flags(LIBRARIES "${LAPACKE_LIBRARIES}" INCLUDE_DIRS "${LAPACKE_INCLUDE_DIRS}")
endif()
