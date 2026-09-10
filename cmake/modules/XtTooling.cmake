# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2016 - 2017, 2020)
#   René Fritze     (2016 - 2020)
#   Tobias Leibner  (2016, 2018 - 2020)
# ~~~

# Converts the path to a source file to a unique name that can be used as a target name. If arg points to a header file,
# the corresponding headercheck target is headercheck_${arg} after calling this function. Example:
# /home/user/dune-xt-super/dune-xt/dune/xt/common/algorithm.hh becomes _dune_xt_common_algorithm.hh
function(dxt_path_to_headercheck_name arg)
  string(REPLACE ${PROJECT_SOURCE_DIR} "" rel ${${arg}})
  string(REGEX REPLACE "/" "_" final ${rel})
  set(${arg}
      ${final}
      PARENT_SCOPE)
endfunction()

# Creates the targets `tidy` and `fix_tidy`, which run clang-tidy over this build's compilation database. All the logic
# lives in .ci/clang_tidy.bash (see its header): the database is pruned to the translation units that exist in the
# repository, clang-tidy itself is pinned to a PyPI wheel and run through `uv run --no-project` the same way the
# coverage targets get gcovr, and three artefacts are written to the build directory -- clang-tidy.log,
# clang-tidy-fixes.yaml and a deduplicated clang-tidy-issues.md.
#
# DXT_TIDY_EXTRA_ARGS forwards further script flags, e.g. --full to include the generated test suites or --filter
# <regex> to narrow the run to a subtree. `fix_tidy` applies the fixes via clang-apply-replacements, which merges the
# duplicate replacements a header collects from every including translation unit -- but review the result: checks such
# as misc-unused-using-decls are inherently per-unit, which is why the NOLINTs in dune/xt/*/print.hh exist.
macro(ADD_TIDY)
  set(DXT_TIDY_VERSION
      "22.1.8"
      CACHE STRING "Version of the pinned clang-tidy wheel used by the tidy targets")
  set(DXT_TIDY_EXTRA_ARGS
      ""
      CACHE STRING "Additional arguments passed to .ci/clang_tidy.bash (e.g. --full, --filter <regex>)")
  # keep the config discoverable from the build tree for editors and clangd
  dune_symlink_to_source_files(FILES .clang-tidy)
  set(_tidy_command ${CMAKE_SOURCE_DIR}/.ci/clang_tidy.bash --build-dir ${CMAKE_BINARY_DIR} --source-dir
                    ${CMAKE_SOURCE_DIR} --uv ${UV_EXECUTABLE} --version ${DXT_TIDY_VERSION} ${DXT_TIDY_EXTRA_ARGS})
  if(NOT TARGET tidy)
    add_custom_target(
      tidy
      COMMAND ${_tidy_command}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running clang-tidy over the compilation database"
      VERBATIM USES_TERMINAL)
  endif()
  if(NOT TARGET fix_tidy)
    add_custom_target(
      fix_tidy
      COMMAND ${_tidy_command} --fix
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      COMMENT "Running clang-tidy over the compilation database and applying its fixes"
      VERBATIM USES_TERMINAL)
  endif()
endmacro()

macro(DEPENDENCYCHECK)
  add_custom_target(dependencycheck SOURCES ${ARGN})
  foreach(header ${ARGN})
    string(REPLACE "/" "_" fn ${header})
    set(TEST_NAME "dependencycheck_${fn}")
    to_list_spaces(CMAKE_CXX_FLAGS TEST_NAME_FLAGS)
    set(XARGS ${TEST_NAME_FLAGS} -DHAVE_CONFIG_H -H -c ${header} -w)
    add_custom_target(${TEST_NAME} + ${dune-gdt_SOURCE_DIR}/cmake/dependencyinfo.py ${CMAKE_CXX_COMPILER} ${XARGS}
                                   ${CMAKE_CURRENT_SOURCE_DIR} ${fn}.dep)
    add_dependencies(dependencycheck ${TEST_NAME})
  endforeach(header)
endmacro(DEPENDENCYCHECK)
