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
# Keep a single compiled extension module as small as it can be.
#
# The wheels ship ~100 of these modules, so anything that is paid per module is paid a hundred times over and the
# PyPI 100 MB per-file limit is reached quickly (it was, see the 145 MB dune.gdt wheel of 2026-08). Two link-time
# knobs cut the per-module cost without touching the generated code:
#
#   --exclude-libs,ALL  Every symbol pulled in from a *static* archive (ALUGrid, OpenBLAS, boost, the dune module
#                       libraries) is otherwise re-exported from the module, even though nothing outside the module
#                       can meaningfully use it -- pybind11 shares its type registry through a Python capsule, not
#                       through the ELF symbol table, and CPython dlopen()s extension modules with RTLD_LOCAL. Across
#                       the 102 modules of the 2026.2.0.571 dune.gdt wheel those exports cost 100 MB of symbol
#                       table, relocation and PLT machinery (16 MB of the 146 MB the wheel compressed down to):
#                       4264 exported symbols per module, of which only 6989 are distinct in total. Localising them
#                       also makes the next flag effective.
#   --gc-sections       With every function and data item in its own section (-ffunction-sections/-fdata-sections)
#                       and the static-archive symbols no longer exported, the linker can drop what this particular
#                       module never reaches. Unreferenced archive members are what the exports were pinning.
#
# Both are GNU-ld/lld options and are only added for ELF targets. --gc-sections is safe for the static
# initialisers the dune/ALUGrid singletons rely on: the default linker script wraps .init_array/.fini_array in
# KEEP(), and the module entry point PyInit_<name> keeps pybind11's own translation unit rooted.
# ~~~
function(dune_pybindxi_minimize_module_size target_name)
  if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    return()
  endif()
  target_compile_options(${target_name} PRIVATE -ffunction-sections -fdata-sections)
  target_link_options(${target_name} PRIVATE "-Wl,--exclude-libs,ALL" "-Wl,--gc-sections")
endfunction()

# ~~~
# Build a Python extension module:
# dune_pybindxi_add_module(<name> [MODULE | SHARED] [EXCLUDE_FROM_ALL] [NO_EXTRAS] [THIN_LTO] source1 [source2 ...])
# Renamed copy of pybind11_add_module, added code blocks are marked with dune-pybindxi START/END.
# ~~~

macro(DUNE_PYBINDXI_ADD_MODULE target_name)
  if(NOT TARGET bindings)
    add_custom_target(bindings)
  endif()
  pybind11_add_module(${target_name} ${ARGN})
  dune_target_link_libraries(${target_name} "${DUNE_LIB_ADD_LIBS}")
  dune_target_enable_all_packages(${target_name})

  target_include_directories(${target_name} PRIVATE ${PYBIND11_INCLUDE_DIR} ${PYTHON_INCLUDE_DIRS})
  dune_pybindxi_minimize_module_size(${target_name})
  add_dependencies(bindings ${target_name})
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
