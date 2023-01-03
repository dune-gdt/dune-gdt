# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2016 - 2017)
#   René Fritze     (2018 - 2020)
#   Tobias Leibner  (2020 - 2021)
#
# The dune_pybind11_add_module function is a renamed copy of pybind11_add_module from
# ../../pybind11/tools/pybind11Tools.cmake, see ../../pybind11/LICENSE for license information.
# ~~~

# ~~~
# Build a Python extension module:
# dune_pybindxi_add_module(<name> [MODULE | SHARED] [EXCLUDE_FROM_ALL] [NO_EXTRAS] [THIN_LTO] source1 [source2 ...])
# Renamed copy of pybind11_add_module, added code blocks are marked with dune-pybindxi START/END.
# ~~~

# cmake-lint: disable=R0912,R0915
macro(DUNE_PYBINDXI_ADD_MODULE target_name)
  pybind11_add_module(${target_name} ${ARGN})
  dune_target_link_libraries(${target_name} "${DUNE_LIB_ADD_LIBS}")
  dune_target_enable_all_packages(${target_name})

  target_include_directories(${target_name} PRIVATE ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
endmacro()

macro(DXT_ADD_MAKE_DEPENDENT_BINDINGS)
  add_custom_target(dependent_bindings)
  if(TARGET bindings AND NOT DXT_NO_AUTO_BINDINGS_DEPENDS)
    add_dependencies(bindings dependent_bindings)
  endif()
  foreach(mod ${ARGN})
    dune_module_path(MODULE ${mod} RESULT ${mod}_binary_dir BUILD_DIR)
    set(tdir ${${mod}_binary_dir})
    if(IS_DIRECTORY ${tdir})
      add_custom_target(${mod}_bindings COMMAND ${CMAKE_COMMAND} --build ${tdir} --target bindings_no_ext -- -j1)
      add_dependencies(dependent_bindings ${mod}_bindings)
    endif()
  endforeach()
endmacro()
