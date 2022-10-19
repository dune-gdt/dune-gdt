# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze    (2018 - 2020, 2022)
#   Tobias Leibner (2018 - 2021)
#
# This file is part of the dune-xt project:
#
# * FindMPI4PY Find mpi4py includes and library This module defines: MPI4PY_INCLUDE_DIR, where to find mpi4py.h, etc.
#   MPI4PY_FOUND
# ~~~

# https://compacc.fnal.gov/projects/repositories/entry/synergia2/CMake/FindMPI4PY.cmake?rev=c147eafb60728606af4fe7b1b16
# 1a660df142e9a
macro(MYFIND_MPI4PY)
  if(MPI4PY_INCLUDE_DIR)
    set(MPI4PY_FOUND TRUE)
  else()
    set(_include_cmd
        "import mpi4py,pathlib; incl=str(mpi4py.get_include()).strip();print(pathlib.Path(incl).resolve().absolute())")
    execute_process(
      COMMAND "${RUN_IN_ENV_SCRIPT}" "python3" "-c" "${_include_cmd}"
      OUTPUT_VARIABLE MPI4PY_INCLUDE_DIR
      RESULT_VARIABLE MPI4PY_COMMAND_RESULT
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(MPI4PY_COMMAND_RESULT)
      execute_process(
        COMMAND "${PYTHON_EXECUTABLE}" "-c" "${_include_cmd}"
        OUTPUT_VARIABLE MPI4PY_INCLUDE_DIR
        RESULT_VARIABLE MPI4PY_COMMAND_RESULT
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    if(MPI4PY_COMMAND_RESULT)
      message(STATUS "mpi4py not found")
      set(MPI4PY_FOUND FALSE)
    else(MPI4PY_COMMAND_RESULT)
      if(MPI4PY_INCLUDE_DIR MATCHES "Traceback")
        message(STATUS "mpi4py matches traceback -> not found")
        # Did not successfully include MPI4PY
        set(MPI4PY_FOUND FALSE)
      else()
        # successful
        set(MPI4PY_FOUND TRUE)
        message(STATUS "mpi4py found: ${MPI4PY_INCLUDE_DIR}")
      endif()
    endif(MPI4PY_COMMAND_RESULT)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(MPI4PY DEFAULT_MSG MPI4PY_INCLUDE_DIR)
  message(STATUS "include dir for MPI4PY ${MPI4PY_INCLUDE_DIR} via ${MPI4PY_COMMAND_RESULT}")
endmacro()

myfind_mpi4py()
