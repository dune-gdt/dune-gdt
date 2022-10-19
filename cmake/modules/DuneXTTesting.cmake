# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2010-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2012 - 2017, 2019 - 2020)
#   René Fritze     (2010 - 2020, 2022)
#   Sven Kaulmann   (2013)
#   Tobias Leibner  (2015 - 2022)
#   felix.albrecht  (2011)
#
# This file is part of the dune-xt project:
# ~~~

include(XtTooling)

macro(GET_HEADERCHECK_TARGETS)
  file(GLOB_RECURSE bindir_header "${CMAKE_BINARY_DIR}/*.hh")
  list(APPEND dxt_ignore_header ${bindir_header})
  if(ENABLE_HEADERCHECK)
    file(GLOB_RECURSE headerlist "${CMAKE_SOURCE_DIR}/dune/*/*.hh" "${CMAKE_SOURCE_DIR}/dune/*/test/*.hh"
         "${CMAKE_SOURCE_DIR}/python/dune/*/*.hh")

    add_custom_target(dxt_headercheck)
    list(FILTER headerlist EXCLUDE REGEX ".*\/deps\/.*")
    foreach(header ${headerlist})
      list(FIND dxt_ignore_header "${header}" _index)
      if(${_index} GREATER -1)
        continue()
      endif() # do some name conversion
      set(targname ${header})
      dxt_path_to_headercheck_name(targname)
      set(targname "headercheck_${targname}")
      list(APPEND dxt_headercheck_targets "${targname}")
      add_dependencies(dxt_headercheck ${targname})
    endforeach(header ${headerlist})
  endif(ENABLE_HEADERCHECK)
endmacro(GET_HEADERCHECK_TARGETS)

# cmake-lint: disable=R0915
macro(ADD_SUBDIR_TESTS subdir)
  get_property(dxt_test_dirs GLOBAL PROPERTY dxt_test_dirs_prop)
  set(dxt_test_dirs ${dxt_test_dirs} ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
  set_property(GLOBAL PROPERTY dxt_test_dirs_prop "${dxt_test_dirs}")
endmacro()

macro(_process_sources test_sources subdir)
  foreach(source ${test_sources})
    set(ranks "1")
    if(source MATCHES "mpi")
      list(APPEND ranks ${DUNE_MAX_TEST_CORES})
    endif(source MATCHES "mpi")
    get_filename_component(testbase ${source} NAME_WE)
    string(REPLACE ".cc" ".mini" minifile ${source})
    if(EXISTS ${minifile})
      if(dune-testtools_FOUND)
        dune_add_system_test(
          SOURCE
          ${source}
          ${COMMON_HEADER}
          INIFILE
          ${minifile}
          BASENAME
          test_${testbase}
          CREATED_TARGETS
          targetlist_${testbase}
          ADDED_TESTS
          testlist_${testbase}
          SCRIPT
          dune_xt_execute.py
          ${DEBUG_MACRO_TESTS})
        foreach(target ${targetlist_${testbase}})
          target_link_libraries(${target} PRIVATE ${link_xt_libs} ${COMMON_LIBS} ${GRID_LIBS} gtest_dune_xt)
          list(APPEND ${subdir}_dxt_test_binaries ${target})
          set(dxt_test_names_${target} ${testlist_${testbase}_${target}})
          foreach(test_name ${dxt_test_names_${target}})
            set_tests_properties(${test_name} PROPERTIES LABELS subdir_${subdir})
          endforeach()
        endforeach(target)
      else(dune-testtools_FOUND)
        message("-- missing dune-testtools, disabling test ${source}")
      endif(dune-testtools_FOUND)
    else(EXISTS ${minifile})
      set(target test_${testbase})
      dune_add_test(
        NAME
        ${target}
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
        --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}.xml
        TIMEOUT
        ${DXT_TEST_TIMEOUT}
        MPI_RANKS
        ${ranks})
      list(APPEND ${subdir}_dxt_test_binaries ${target})
      set(dxt_test_names_${target} ${target})
      set_tests_properties(${target} PROPERTIES LABELS subdir_${subdir})
    endif(EXISTS ${minifile})
  endforeach(source)
endmacro()

macro(_PROCESS_SUBDIR_TESTS fullpath)
  set(link_xt_libs dunext)

  if(NOT DXT_TEST_TIMEOUT)
    set(DXT_TEST_TIMEOUT 1000)
  endif()

  get_filename_component(subdir ${fullpath} NAME)
  file(GLOB_RECURSE test_sources "${fullpath}/*.cc")

  if(NOT test_sources)
    message(AUTHOR_WARNING "called add_subdir_test(${subdir}), but no sources were found")
  endif()

  _process_sources("${test_sources}" "${subdir}")

  file(GLOB_RECURSE test_templates "${CMAKE_CURRENT_SOURCE_DIR}/${subdir}/*.tpl")
  foreach(template ${test_templates})
    set(ranks "1")
    if(template MATCHES "mpi")
      list(APPEND ranks ${DUNE_MAX_TEST_CORES})
    endif(template MATCHES "mpi")
    get_filename_component(testbase ${template} NAME_WE)
    string(REPLACE ".tpl" ".py" config_fn "${template}")
    string(REPLACE ".tpl" ".tpl.cc" out_fn "${template}")
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}" out_fn "${out_fn}")
    # get the last completed cache for the codegen execution during configure time
    foreach(mod ${ALL_DEPENDENCIES})
      dune_module_path(MODULE ${mod} RESULT ${mod}_binary_dir BUILD_DIR)
      if(IS_DIRECTORY ${${mod}_binary_dir})
        set(last_dep_bindir ${${mod}_binary_dir})
      endif()
    endforeach(mod DEPENDENCIES)

    # TODO dxt_code_geneartion.py should be avail in the venv when called from this target
    dune_execute_process(
      COMMAND
      ${RUN_IN_ENV_SCRIPT}
      ${PROJECT_SOURCE_DIR}/python/scripts/dxt_code_generation.py
      "${config_fn}"
      "${template}"
      "${CMAKE_BINARY_DIR}"
      "${out_fn}"
      "${last_dep_bindir}"
      OUTPUT_VARIABLE
      codegen_output)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/codegen.${testbase}.log" ${codegen_output})
    file(GLOB generated_sources "${out_fn}.*")
    if("" STREQUAL "${generated_sources}")
      set(generated_sources ${out_fn})
    endif()
    add_custom_command(
      OUTPUT "${generated_sources}"
      COMMAND ${RUN_IN_ENV_SCRIPT} ${PROJECT_SOURCE_DIR}/python/scripts/dxt_code_generation.py "${config_fn}"
              "${template}" "${CMAKE_BINARY_DIR}" "${out_fn}" "${last_dep_bindir}"
      DEPENDS "${config_fn}" "${template}"
      VERBATIM USES_TERMINAL)
    foreach(gen_source ${generated_sources})
      string(REPLACE "${out_fn}." "" postfix "${gen_source}")
      string(REPLACE "${out_fn}" "" postfix "${postfix}")
      string(REPLACE ".cc" "" postfix "${postfix}")
      if(NOT "" STREQUAL "${postfix}")
        set(postfix "__${postfix}")
      endif()
      set(target test_${testbase}${postfix})
      dune_add_test(
        NAME
        ${target}
        SOURCES
        ${gen_source}
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
        --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}.xml
        TIMEOUT
        ${DXT_TEST_TIMEOUT}
        MPI_RANKS
        ${ranks})
      list(APPEND ${subdir}_dxt_test_binaries ${target})
      set(dxt_test_names_${target} ${target})
      set_tests_properties(${target} PROPERTIES LABELS subdir_${subdir})
    endforeach()
  endforeach(template ${test_templates})
  add_custom_target(${subdir}_test_templates SOURCES ${test_templates})

  # this excludes meta-ini variation test cases because there binary name != test name
  foreach(test ${${subdir}_dxt_test_binaries})
    if(TEST test)
      set_tests_properties(${test} PROPERTIES TIMEOUT ${DXT_TEST_TIMEOUT})
      set_tests_properties(${test} PROPERTIES LABELS subdir_${subdir})
    endif(TEST test)
  endforeach()

  add_custom_target(${subdir}_test_binaries DEPENDS ${${subdir}_dxt_test_binaries})

  add_custom_target(
    ${subdir}_check
    COMMAND ${CMAKE_CTEST_COMMAND} --timeout ${DXT_TEST_TIMEOUT} -j ${DXT_TEST_PROCS}
    DEPENDS ${subdir}_test_binaries
    USES_TERMINAL)
  add_custom_target(
    ${subdir}_recheck
    COMMAND ${CMAKE_CTEST_COMMAND} --timeout ${DXT_TEST_TIMEOUT} --rerun-failed -j ${DXT_TEST_PROCS}
    DEPENDS ${subdir}_test_binaries
    USES_TERMINAL)
  foreach(target ${${subdir}_dxt_test_binaries})
    set(all_sorted_testnames "${all_sorted_testnames}/${dxt_test_names_${target}}")
  endforeach()
endmacro(_PROCESS_SUBDIR_TESTS)

macro(FINALIZE_TEST_SETUP)
  get_headercheck_targets()
  get_property(dxt_test_dirs GLOBAL PROPERTY dxt_test_dirs_prop)
  set(combine_targets test_templates test_binaries check recheck)

  foreach(target ${combine_targets})
    add_custom_target(${target})
  endforeach()

  foreach(fullpath ${dxt_test_dirs})
    _process_subdir_tests(${fullpath})
    get_filename_component(subdir ${fullpath} NAME)

    foreach(target ${combine_targets})
      add_dependencies(${target} ${subdir}_${target})
    endforeach()

    set(dxt_test_binaries "${dxt_test_binaries} ${${subdir}_dxt_test_binaries}")
  endforeach()

  if(ALBERTA_FOUND)
    foreach(test ${dxt_test_binaries})
      if(${test} MATCHES alberta_1d)
        add_dune_alberta_flags(GRIDDIM 1 ${test})
      elseif(${test} MATCHES alberta_2d)
        add_dune_alberta_flags(GRIDDIM 2 ${test})
      elseif(${test} MATCHES alberta_3d)
        add_dune_alberta_flags(GRIDDIM 3 ${test})
      endif()
    endforeach()

    foreach(test ${dxt_test_binaries})
      if(${test} MATCHES 2d_simplex_alberta)
        add_dune_alberta_flags(GRIDDIM 2 ${test})
      elseif(${test} MATCHES 3d_simplex_alberta)
        add_dune_alberta_flags(GRIDDIM 3 ${test})
      endif()
    endforeach()
  endif()
endmacro(FINALIZE_TEST_SETUP)

macro(DXT_EXCLUDE_FROM_HEADERCHECK)
  exclude_from_headercheck(${ARGV0}) # make this robust to argument being passed with or without ""
  string(REGEX REPLACE "[\ \n]+([^\ ])" ";\\1" list ${ARGV0})
  set(list "${list};${ARGV}")
  foreach(item ${list})
    set(item ${CMAKE_CURRENT_SOURCE_DIR}/${item})
    list(APPEND dxt_ignore_header ${item})
  endforeach()
endmacro(DXT_EXCLUDE_FROM_HEADERCHECK)

macro(DXT_ADD_PYTHON_TESTS)
  add_custom_target(
    xt_test_python
    "${RUN_IN_ENV_SCRIPT}"
    "python"
    "-m"
    "pytest"
    "${CMAKE_BINARY_DIR}/python/xt"
    "--cov"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    "--junitxml=${CMAKE_BINARY_DIR}/pytest_results_xt.xml"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/python/xt"
    DEPENDS bindings
    VERBATIM USES_TERMINAL)
  add_custom_target(
    gdt_test_python
    "${RUN_IN_ENV_SCRIPT}"
    "python"
    "-m"
    "pytest"
    "${CMAKE_BINARY_DIR}/python/gdt"
    "--cov"
    "${CMAKE_CURRENT_SOURCE_DIR}/"
    "--junitxml=${CMAKE_BINARY_DIR}/pytest_results_gdt.xml"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/python/gdt"
    DEPENDS bindings
    VERBATIM USES_TERMINAL)
  if(NOT TARGET test_python)
    add_custom_target(test_python)
  endif(NOT TARGET test_python)
  add_dependencies(test_python xt_test_python gdt_test_python)
endmacro(DXT_ADD_PYTHON_TESTS)
