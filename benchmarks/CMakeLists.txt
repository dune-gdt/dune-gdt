# ~~~
# This file is part of the dune-gdt project:
#   https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# ~~~

# nanobench is a single-header micro-benchmarking library, pulled in via vcpkg (see vcpkg.json).
find_package(nanobench CONFIG REQUIRED)

# Any C++ file dropped into this directory is automatically built as a standalone benchmark executable named
# bench_<filename>. The benchmarks are plain `int main()` programs (no gtest, no CTest registration) that drive the
# public dune-gdt API and emit nanobench JSON reports.
file(GLOB_RECURSE benchmark_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
     "${CMAKE_CURRENT_SOURCE_DIR}/*.cc")

set(benchmark_targets)
set(benchmark_report_dir "${CMAKE_BINARY_DIR}/benchmarks/reports")

foreach(source ${benchmark_sources})
  get_filename_component(_name ${source} NAME_WE)
  set(_target "bench_${_name}")
  # EXCLUDE_FROM_ALL: benchmarks are only built on demand (target `benchmarks`), never as part of the default build or
  # the test binaries.
  add_executable(${_target} EXCLUDE_FROM_ALL ${source})
  dune_target_enable_all_packages(${_target})
  target_link_libraries(${_target} PRIVATE dunext ${GRID_LIBS} nanobench::nanobench)
  list(APPEND benchmark_targets ${_target})
endforeach()

# `benchmarks`: build every benchmark executable.
add_custom_target(
  benchmarks
  DEPENDS ${benchmark_targets}
  COMMENT "Build all dune-gdt benchmarks")

# `run_benchmarks`: build, then run every benchmark, writing one <name>.json nanobench report per benchmark into
# ${benchmark_report_dir}.
set(_run_commands COMMAND ${CMAKE_COMMAND} -E make_directory ${benchmark_report_dir})
foreach(_target ${benchmark_targets})
  list(
    APPEND
    _run_commands
    COMMAND
    ${CMAKE_COMMAND}
    -E
    env
    "DUNE_GDT_BENCHMARK_OUTPUT_DIR=${benchmark_report_dir}"
    $<TARGET_FILE:${_target}>)
endforeach()
add_custom_target(
  run_benchmarks
  ${_run_commands}
  DEPENDS benchmarks
  USES_TERMINAL
  COMMENT "Run all dune-gdt benchmarks and write nanobench JSON reports to ${benchmark_report_dir}")
