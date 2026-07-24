# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2012 - 2017, 2019 - 2020)
#   René Fritze     (2010 - 2020)
#   Sven Kaulmann   (2013)
#   Tobias Leibner  (2015 - 2020)
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
          ${CMAKE_BINARY_DIR}/python/xt/wrapper/dune_xt_execute.py
          ${DEBUG_MACRO_TESTS})
        foreach(target ${targetlist_${testbase}})
          target_link_libraries(${target} PRIVATE ${link_xt_libs} ${COMMON_LIBS} ${GRID_LIBS} gtest_dune_xt)
          list(APPEND ${subdir}_dxt_test_binaries ${target})
          set(dxt_test_names_${target} ${testlist_${testbase}_${target}})
          foreach(test_name ${dxt_test_names_${target}})
            set_tests_properties(${test_name} PROPERTIES LABELS "subdir_${subdir} dune-gdt-test")
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
        CMD_ARGS
        --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}.xml
        TIMEOUT
        ${DXT_TEST_TIMEOUT}
        MPI_RANKS
        ${ranks}
        LABELS
        dune-gdt-test
        subdir_${subdir})
      list(APPEND ${subdir}_dxt_test_binaries ${target})
      set(dxt_test_names_${target} ${target})
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

  # Glob the templated suites relative to ${fullpath} (the full path add_subdir_tests() recorded), exactly as the .cc
  # glob above does. This used to splice ${subdir} onto ${CMAKE_CURRENT_SOURCE_DIR}, which is dune/ here (this macro
  # runs from finalize_test_setup(), invoked in dune/CMakeLists.txt), so it looked for dune/functions/*.tpl,
  # dune/la/*.tpl, ... -- directories that do not exist. All 49 templated suites were silently skipped (issue #370).
  file(GLOB_RECURSE test_templates "${fullpath}/*.tpl")
  # Record what was picked up so finalize_test_setup() can cross-check it against the whole tree (see the guard there).
  get_property(dxt_seen_templates GLOBAL PROPERTY dxt_seen_test_templates_prop)
  list(APPEND dxt_seen_templates ${test_templates})
  set_property(GLOBAL PROPERTY dxt_seen_test_templates_prop "${dxt_seen_templates}")

  # Both the configure-time expansion below (CMake needs the generated file list to declare the targets) and the
  # build-time regeneration in add_custom_command run the same command:
  #
  # * `uv run --no-project --with ...` supplies jinja2/pyparsing without a manually-managed venv, the same way the
  #   coverage targets in dxt_add_python_tests() pull gcovr/coverage.py, and `--python ${Python_EXECUTABLE}` pins the
  #   interpreter the rest of the build resolved (see the uv block in the top-level CMakeLists.txt).
  # * PYTHONPATH points at the binary-dir assembly of the `dune.xt` package -- the symlinked sources plus the configured
  #   _version.py that dune_pybindxi_install_python_package() and python/xt/dune/xt/CMakeLists.txt put there. The
  #   per-suite .py configs import dune.xt.codegen / dune.xt.test.grid_types, and the source tree on its own is not
  #   importable because dune/xt/__init__.py imports the generated _version. add_subdirectory(python) precedes
  #   add_subdirectory(dune) in the top-level CMakeLists.txt, so the assembly exists by the time we get here.
  #
  # This replaces the former ${RUN_IN_ENV_SCRIPT} wrapper, which has been unset ever since the dune-testtools virtualenv
  # was dropped (its find_program is commented out in CMakeLists.txt), and the ${PROJECT_SOURCE_DIR}/python/ scripts/
  # path, which has never existed -- the script lives in python/xt/scripts/.
  set(dxt_codegen_command
      ${CMAKE_COMMAND} -E env "PYTHONPATH=${CMAKE_BINARY_DIR}/python/xt" uv run --no-project --with jinja2 --with
      pyparsing --python ${Python_EXECUTABLE} python ${PROJECT_SOURCE_DIR}/python/xt/scripts/dxt_code_generation.py)

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

    dune_execute_process(
      COMMAND
      ${dxt_codegen_command}
      "${config_fn}"
      "${template}"
      "${CMAKE_BINARY_DIR}"
      "${out_fn}"
      "${last_dep_bindir}"
      OUTPUT_VARIABLE
      codegen_output
      ERROR_MESSAGE
      "failed to expand the templated test suite ${template}")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/codegen.${testbase}.log" "${codegen_output}")
    file(GLOB generated_sources "${out_fn}.*")
    if("" STREQUAL "${generated_sources}")
      set(generated_sources ${out_fn})
    endif()
    add_custom_command(
      OUTPUT "${generated_sources}"
      COMMAND ${dxt_codegen_command} "${config_fn}" "${template}" "${CMAKE_BINARY_DIR}" "${out_fn}" "${last_dep_bindir}"
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
        CMD_ARGS
        --gtest_output=xml:${CMAKE_CURRENT_BINARY_DIR}/${target}.xml
        TIMEOUT
        ${DXT_TEST_TIMEOUT}
        MPI_RANKS
        ${ranks}
        LABELS
        dune-gdt-test
        subdir_${subdir})
      list(APPEND ${subdir}_dxt_test_binaries ${target})
      set(dxt_test_names_${target} ${target})
    endforeach()
  endforeach(template ${test_templates})
  add_custom_target(${subdir}_test_templates SOURCES ${test_templates})

  # A loop used to re-apply TIMEOUT and LABELS to every ${subdir}_dxt_test_binaries entry here, guarded by `if(TEST
  # test)` -- the unexpanded literal `test`, so it asked whether a ctest test named "test" exists and the body never
  # ran. It is gone rather than repaired: both branches above now pass TIMEOUT and the full label set (dune-gdt-test
  # plus subdir_${subdir}) straight to dune_add_test/dune_add_system_test, and re-setting LABELS here would clobber
  # dune-gdt-test -- the label every ctest preset filters on -- which is precisely the bug this loop's sibling at the
  # .tpl branch caused (issue #370).

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

    list(APPEND dxt_test_binaries "${${subdir}_dxt_test_binaries}")
  endforeach()

  # Regression guard for issue #370: the per-subdir *.tpl glob resolved to a non-existent path for years, so all 49
  # templated suites were skipped without a word -- there was no counterpart to the empty-test_sources AUTHOR_WARNING in
  # _process_subdir_tests(). Warning per subdir would fire for every dune/gdt/test subdir (none of which has templates),
  # so the check is made once, here: every *.tpl file in the module must have been picked up by one of the per-subdir
  # globs. This also catches a suite added under a directory nobody passed to add_subdir_tests().
  file(GLOB_RECURSE dxt_all_templates "${PROJECT_SOURCE_DIR}/dune/*.tpl")
  get_property(dxt_seen_templates GLOBAL PROPERTY dxt_seen_test_templates_prop)
  list(LENGTH dxt_all_templates dxt_all_templates_count)
  list(LENGTH dxt_seen_templates dxt_seen_templates_count)
  if(NOT dxt_all_templates_count EQUAL dxt_seen_templates_count)
    message(
      AUTHOR_WARNING
        "found ${dxt_all_templates_count} templated (*.tpl) test suites under dune/, but add_subdir_tests() picked up "
        "only ${dxt_seen_templates_count} -- the rest generate no tests and contribute no coverage")
  endif()

  if(Alberta_FOUND)
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

  if(Alberta_FOUND)
    foreach(test ${dxt_test_binaries})
      # this makes sure `libtirpc` is linked LAST
      set_target_properties(${test} PROPERTIES LINK_FLAGS "-Wl,--whole-archive ${TIRPC_LIB} -Wl,--no-whole-archive")
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
  # The Python test suites are registered as CTest tests (run by `ctest`, never as standalone build targets) so a single
  # `ctest --preset ...` exercises both the C++ and the Python suites. Each runs through `uv run`, pinned to the very
  # interpreter the bindings were compiled against (${Python_EXECUTABLE}, so the cpython ABI of the built .so modules
  # matches). uv runs in project mode: the test's WORKING_DIRECTORY is the binary-dir assembly point holding the
  # symlinked pyproject.toml, the configured _version.py and the compiled .so modules, so it is a complete editable
  # project from uv's point of view. The full set of test dependencies is declared once as the PEP 735 `test` dependency
  # group in each package's pyproject.toml and pulled in with `--group test` (rather than individual `--with` flags).
  # dune.gdt depends on the exact-version dune.xt (and its extras re-export the same pin); python/gdt/pyproject.toml
  # routes every dune.xt requirement to the freshly built editable in the sibling binary dir via [tool.uv.sources], so
  # resolution is satisfied from the local build rather than an index without needing `--with-editable`. The bindings
  # .so modules must be built before `ctest` runs (CTest does not build dependencies): build the `bindings` target
  # first.
  #
  # `--frozen` pins resolution to the committed lockfile (python/{xt,gdt}/uv.lock, symlinked into the assembly point
  # alongside pyproject.toml). It is used as-is: uv neither re-resolves against an index nor writes the lock back
  # through that symlink into the source tree, keeping the test run reproducible and offline. Regenerate the lockfiles
  # from a clean source checkout with `python python/update_lockfiles.py` (no build dir required).
  add_test(
    NAME xt_test_python
    COMMAND uv run --frozen --python ${Python_EXECUTABLE} --group test python -m pytest ${CMAKE_BINARY_DIR}/python/xt
            --cov ${CMAKE_CURRENT_SOURCE_DIR}/ --junitxml=${CMAKE_BINARY_DIR}/pytest_results_xt.xml)
  set_tests_properties(
    xt_test_python PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python/xt ENVIRONMENT
                              COVERAGE_FILE=${CMAKE_BINARY_DIR}/coverage-xt LABELS "dune-gdt-test;python_test")
  add_test(
    NAME gdt_test_python
    COMMAND uv run --frozen --python ${Python_EXECUTABLE} --group test python -m pytest ${CMAKE_BINARY_DIR}/python/gdt
            --cov ${CMAKE_CURRENT_SOURCE_DIR}/ --junitxml=${CMAKE_BINARY_DIR}/pytest_results_gdt.xml)
  set_tests_properties(
    gdt_test_python PROPERTIES WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python/gdt ENVIRONMENT
                               COVERAGE_FILE=${CMAKE_BINARY_DIR}/coverage-gdt LABELS "dune-gdt-test;python_test")

  # Coverage-processing targets (moved here from the CI workflow). Run them after `ctest`: the pytest tests above write
  # the coverage.py data files (coverage-xt, coverage-gdt) into the build dir, and the instrumented C++ tests (the
  # release_coverage preset) write the gcov .gcda/.gcno files under the build tree. Both gcovr and coverage.py are
  # pulled on the fly by uv (no manually-managed venv); they are also listed in the `infrastructure` dev group in
  # python/xt/pyproject.toml.
  if(NOT TARGET coverage_cpp)
    # gcov data under the build tree -> Cobertura XML codecov understands, filtered to our own dune/ sources.
    add_custom_target(
      coverage_cpp
      COMMAND
        uv run --no-project --with gcovr gcovr --root ${CMAKE_SOURCE_DIR} --filter ${CMAKE_SOURCE_DIR}/dune/
        --gcov-ignore-parse-errors --exclude-unreachable-branches --exclude-throw-branches --print-summary --xml-pretty
        -o ${CMAKE_BINARY_DIR}/coverage-cpp.xml ${CMAKE_BINARY_DIR}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM USES_TERMINAL)
  endif()
  if(NOT TARGET coverage_cpp_llvm)
    # llvm source-based coverage (clang22-release_coverage preset: -fprofile-instr-generate -fcoverage-mapping) ->
    # lcov-format reports, one with llvm's branch records and one line-only, plus a per-area markdown summary of each
    # for comparison against the gcc/gcovr numbers (issue #314). DXT_LLVM_VERSION picks the llvm-profdata/llvm-cov/
    # llvm-objdump suffix; the preset sets it explicitly, the default matches the current clang22-* presets.
    set(DXT_LLVM_VERSION
        "22"
        CACHE STRING "major version suffix of the llvm-profdata/llvm-cov/llvm-objdump binaries")
    add_custom_target(
      coverage_cpp_llvm
      COMMAND ${CMAKE_SOURCE_DIR}/.ci/llvm_cov_export.bash ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR} ${DXT_LLVM_VERSION}
      COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/.ci/lcov_area_summary.py ${CMAKE_BINARY_DIR}/coverage-cpp.info
              --root ${CMAKE_SOURCE_DIR}
      COMMAND ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/.ci/lcov_area_summary.py
              ${CMAKE_BINARY_DIR}/coverage-cpp-lineonly.info --root ${CMAKE_SOURCE_DIR}
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM USES_TERMINAL)
  endif()
  if(NOT TARGET coverage_python)
    # combine the two coverage.py data files written by the pytest CTest runs and emit one XML.
    add_custom_target(
      coverage_python
      COMMAND uv run --no-project --with coverage python -m coverage combine coverage-xt coverage-gdt
      COMMAND uv run --no-project --with coverage python -m coverage xml -o ${CMAKE_BINARY_DIR}/coverage-python.xml
      WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
      VERBATIM USES_TERMINAL)
  endif()
  if(NOT TARGET coverage)
    add_custom_target(coverage DEPENDS coverage_cpp coverage_python)
  endif()

  # test_python is kept as a (now empty) aggregate build target for backwards compatibility: the xt/gdt Python suites it
  # used to drive are registered as CTest tests above and are run via `ctest`, not as standalone build targets.
  if(NOT TARGET test_python)
    add_custom_target(test_python)
  endif(NOT TARGET test_python)
endmacro(DXT_ADD_PYTHON_TESTS)
