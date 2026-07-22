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

message("-- checking for lapacke library")
find_library(LAPACKE_LIBRARY lapacke HINTS ${LIB_HINTS})

find_package(BLAS)
find_package(LAPACK)

if("${LAPACKE_LIBRARY}" MATCHES "LAPACKE_LIBRARY-NOTFOUND")
  # Standalone liblapacke not found; vcpkg's OpenBLAS bundles LAPACKE inside libopenblas (built static to avoid AVX512
  # dynamic-kernel failures on gcc-13), so try that as a fallback -- the LAPACKE_* symbols and lapacke.h header are
  # present inside it.
  find_library(_lapacke_openblas_fallback openblas HINTS ${LIB_HINTS})
  if(NOT "${_lapacke_openblas_fallback}" MATCHES "_lapacke_openblas_fallback-NOTFOUND")
    include(CheckLibraryExists)
    # LAPACKE_dsygv wraps Fortran-compiled code; gfortran (and BLAS when available) are needed at link time.
    set(_prev_cmake_required_libraries "${CMAKE_REQUIRED_LIBRARIES}")
    set(CMAKE_REQUIRED_LIBRARIES gfortran)
    if(BLAS_FOUND)
      list(APPEND CMAKE_REQUIRED_LIBRARIES ${BLAS_LIBRARIES})
    endif()
    check_library_exists("${_lapacke_openblas_fallback}" LAPACKE_dsygv "" _lapacke_openblas_has_dsygv)
    set(CMAKE_REQUIRED_LIBRARIES "${_prev_cmake_required_libraries}")
    unset(_prev_cmake_required_libraries)
    if(_lapacke_openblas_has_dsygv)
      message("--   standalone LAPACKE not found; using OpenBLAS as LAPACKE provider")
      set(LAPACKE_LIBRARY
          "${_lapacke_openblas_fallback}"
          CACHE PATH "Path to the LAPACKE library" FORCE)
    else()
      message("--   library 'LAPACKE' not found, make sure you have both LAPACK and LAPACKE installed")
    endif()
    unset(_lapacke_openblas_has_dsygv CACHE)
  else()
    message("--   library 'LAPACKE' not found, make sure you have both LAPACK and LAPACKE installed")
  endif()
  unset(_lapacke_openblas_fallback CACHE)
else()
  message("--   found LAPACKE library")
endif()
if(NOT "${LAPACKE_LIBRARY}" MATCHES "LAPACKE_LIBRARY-NOTFOUND")
  set(LAPACKE_LIBRARIES "${LAPACKE_LIBRARY}")
endif()

message("-- checking for lapacke.h header")
set(LAPACKE_HEADER_INCLUDE_HINTS "")
append_to_each("${INCLUDE_HINTS}" "lapacke/" LAPACKE_HEADER_INCLUDE_HINTS)
append_to_each("${INCLUDE_HINTS}" "openblas/" LAPACKE_HEADER_INCLUDE_HINTS)
list(APPEND LAPACKE_HEADER_INCLUDE_HINTS ${INCLUDE_HINTS})
find_path(LAPACKE_INCLUDE_DIRS lapacke.h HINTS ${LAPACKE_HEADER_INCLUDE_HINTS})
if("${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")
  message("--   lapacke.h header not found")
else("${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")
  message("--   found lapacke.h header")
  include_sys_dir("${LAPACKE_INCLUDE_DIRS}")
endif("${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")

if(BLAS_FOUND)
  list(APPEND LAPACKE_LIBRARIES ${BLAS_LIBRARIES})
  list(APPEND LAPACKE_LIBRARIES gfortran)
endif()
if(LAPACK_FOUND)
  list(APPEND LAPACKE_LIBRARIES ${LAPACK_LIBRARIES})
endif()

if(NOT DEFINED LAPACKE_FOUND)
  set(LAPACKE_FOUND 0)
endif(NOT DEFINED LAPACKE_FOUND)
if(LAPACK_FOUND)
  if(NOT "${LAPACKE_LIBRARY}" MATCHES "LAPACKE_LIBRARY-NOTFOUND")
    if(NOT "${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")
      set(LAPACKE_FOUND 1)
    endif(NOT "${LAPACKE_INCLUDE_DIRS}" MATCHES "LAPACKE_INCLUDE_DIRS-NOTFOUND")
  endif(NOT "${LAPACKE_LIBRARY}" MATCHES "LAPACKE_LIBRARY-NOTFOUND")
endif(LAPACK_FOUND)

set(HAVE_LAPACKE ${LAPACKE_FOUND})

# register all LAPACKE related flags
if(LAPACKE_FOUND)
  dune_register_package_flags(LIBRARIES "${LAPACKE_LIBRARIES}" INCLUDE_DIRS "${LAPACKE_INCLUDE_DIRS}")
endif()
