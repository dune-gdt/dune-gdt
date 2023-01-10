# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2012 - 2013, 2018)
#   René Fritze     (2018)
#   Tobias Leibner  (2018)
# ~~~

# Removes directory path, .cc suffix and leading __, replaces / with __
macro(CLEANUP_TARGET_NAME arg)
  string(REPLACE ${PROJECT_SOURCE_DIR} "" _targname ${arg})
  string(REPLACE ".cc" "" _targname ${_targname})
  string(REPLACE ".py" "" _targname ${_targname})
  string(REGEX REPLACE "/" "__" _targname ${_targname})
  string(REGEX REPLACE "^__" "" _targname ${_targname})
endmacro(CLEANUP_TARGET_NAME)

file(GLOB_RECURSE example_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")
# remove this list again https://github.com/dune-gdt/dune-merged/issues/33
set(EXAMPLES_IGNORE_LIST
    ${CMAKE_CURRENT_SOURCE_DIR}/stationary-heat-equation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/mpi_2019_02_talk_on_hyperbolic_equations.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/adaptive-swipdg.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/adaptive_elliptic_swipdg.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/adaptive_interpolation.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/gamm_2019_talk_on_conservative_rb_OS2015_academic_example.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gamm_2019_talk_on_conservative_rb_OS2015_multiscale_example.py)

foreach(source ${example_sources}) # do some name conversion
  if(source IN_LIST EXAMPLES_IGNORE_LIST)
    continue()
  endif()
  set(_targname ${source})
  cleanup_target_name(${_targname})
  list(APPEND examples_targets ${_targname})
  dune_add_test(
    NAME
    ${_targname}
    SOURCES
    ${source}
    ${COMMON_HEADER}
    LINK_LIBRARIES
    ${link_xt_libs}
    ${COMMON_LIBS}
    ${GRID_LIBS}
    gtest_dune_xt
    COMMAND
    ${RUN_IN_ENV_SCRIPT}
    CMD_ARGS
    ${CMAKE_CURRENT_BINARY_DIR}/${target}
    --gtest_output=xml:${CMAKE_BINARY_DIR}/dune/examples_${target}.xml
    TIMEOUT
    ${DXT_TEST_TIMEOUT}
    MPI_RANKS
    1)
  set_tests_properties(${_targname} PROPERTIES TIMEOUT ${DXT_TEST_TIMEOUT})
  set_tests_properties(${_targname} PROPERTIES LABELS subdir_examples)
endforeach(source ${example_sources})

add_custom_target(
  install_example_requirements
  COMMAND ${RUN_IN_ENV_SCRIPT} python -m pip install -r ${CMAKE_CURRENT_SOURCE_DIR}/requirements.txt
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE example_sources "${CMAKE_CURRENT_SOURCE_DIR}/*.py")

foreach(source ${example_sources}) # do some name conversion
  if(source IN_LIST EXAMPLES_IGNORE_LIST)
    continue()
  endif()
  set(_targname ${source})
  cleanup_target_name(${_targname})
  list(APPEND examples_targets ${_targname})
  add_custom_target(
    ${_targname}
    COMMAND ${RUN_IN_ENV_SCRIPT} python ${source}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
  add_dependencies(${_targname} install_example_requirements)

  # produces error:

  # dune_add_test( TARGET ${_targname} )

  set_tests_properties(${test} PROPERTIES TIMEOUT ${DXT_TEST_TIMEOUT})
  set_tests_properties(${test} PROPERTIES LABELS subdir_examples)
endforeach(source ${example_sources})

add_custom_target(examples COMMENT "Selected examples showing some of dune-gdt's features.")
add_dependencies(examples ${examples_targets})
