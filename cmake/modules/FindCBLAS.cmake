# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2018 - 2019, 2026)
#   Tobias Leibner (2018, 2020)
# ~~~

include(DuneXTHints)

message("-- checking for cblas library")
find_library(CBLAS_LIBRARY cblas HINTS ${LIB_HINTS})
if("${CBLAS_LIBRARY}" MATCHES "CBLAS_LIBRARY-NOTFOUND")
  # Standalone libcblas not found; vcpkg's OpenBLAS bundles CBLAS inside libopenblas
  # (built static to avoid AVX512 dynamic-kernel failures on gcc-13), so try that as a
  # fallback -- the cblas_* symbols and cblas.h header are present inside it.
  find_library(_cblas_openblas_fallback openblas HINTS ${LIB_HINTS})
  if(NOT "${_cblas_openblas_fallback}" MATCHES "_cblas_openblas_fallback-NOTFOUND")
    message("--   standalone CBLAS not found; using OpenBLAS as CBLAS provider")
    set(CBLAS_LIBRARY "${_cblas_openblas_fallback}" CACHE PATH "Path to the CBLAS library" FORCE)
  else()
    message("--   CBLAS library not found, make sure you have CBLAS installed")
  endif()
  unset(_cblas_openblas_fallback CACHE)
else()
  message("--   found CBLAS library")
endif()
if(NOT "${CBLAS_LIBRARY}" MATCHES "CBLAS_LIBRARY-NOTFOUND")
  set(CBLAS_LIBRARIES "${CBLAS_LIBRARY}")
endif()

message("-- checking for cblas.h header")
set(CBLAS_HEADER_INCLUDE_HINTS "")
append_to_each("${INCLUDE_HINTS}" "openblas/" CBLAS_HEADER_INCLUDE_HINTS)
list(APPEND CBLAS_HEADER_INCLUDE_HINTS ${INCLUDE_HINTS})
find_path(CBLAS_INCLUDE_DIRS cblas.h HINTS ${CBLAS_HEADER_INCLUDE_HINTS})
if("${CBLAS_INCLUDE_DIRS}" MATCHES "CBLAS_INCLUDE_DIRS-NOTFOUND")
  message("--   cblas.h header not found")
else("${CBLAS_INCLUDE_DIRS}" MATCHES "CBLAS_INCLUDE_DIRS-NOTFOUND")
  message("--   found cblas.h header")
  include_sys_dir("${CBLAS_INCLUDE_DIRS}")
endif("${CBLAS_INCLUDE_DIRS}" MATCHES "CBLAS_INCLUDE_DIRS-NOTFOUND")

if(NOT DEFINED CBLAS_FOUND)
  set(CBLAS_FOUND 0)
endif(NOT DEFINED CBLAS_FOUND)
if(NOT "${CBLAS_LIBRARY}" MATCHES "CBLAS_LIBRARY-NOTFOUND")
  if(NOT "${CBLAS_INCLUDE_DIRS}" MATCHES "CBLAS_INCLUDE_DIRS-NOTFOUND")
    set(CBLAS_FOUND 1)
  endif(NOT "${CBLAS_INCLUDE_DIRS}" MATCHES "CBLAS_INCLUDE_DIRS-NOTFOUND")
endif(NOT "${CBLAS_LIBRARY}" MATCHES "CBLAS_LIBRARY-NOTFOUND")

set(HAVE_CBLAS ${CBLAS_FOUND})

# register all CBLAS related flags
if(CBLAS_FOUND)
  dune_register_package_flags(LIBRARIES "${CBLAS_LIBRARIES}" INCLUDE_DIRS "${CBLAS_INCLUDE_DIRS}")
endif()
