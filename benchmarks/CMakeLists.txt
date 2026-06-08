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

set(_benchmark_targets)
set(_benchmark_report_dir "${CMAKE_BINARY_DIR}/benchmarks/reports")

foreach(source ${benchmark_sources})
  get_filename_component(_name ${source} NAME_WE)
  set(_target "bench_${_name}")
  # EXCLUDE_FROM_ALL: benchmarks are only built on demand (target `benchmarks`), never as part of the default build or
  # the test binaries.
  add_executable(${_target} EXCLUDE_FROM_ALL ${source})
  dune_target_enable_all_packages(${_target})
  target_link_libraries(${_target} PRIVATE dunext ${GRID_LIBS} nanobench::nanobench)
  list(APPEND _benchmark_targets ${_target})
endforeach()

# `benchmarks`: build every benchmark executable.
add_custom_target(
  benchmarks
  DEPENDS ${_benchmark_targets}
  COMMENT "Build all dune-gdt benchmarks")

# `run_benchmarks`: build, then run every benchmark, writing one <name>.json nanobench report per benchmark into
# ${_benchmark_report_dir}.
#
# Each benchmark is wrapped in a per-benchmark wall-clock timeout (coreutils `timeout`, if available) so a single
# runaway benchmark fails fast with a clear error instead of consuming the whole CI job timeout. `timeout` exits 124 on
# expiry, which fails the custom command (and thus the `run_benchmarks` target). If `timeout` is not found (e.g. on some
# macOS dev setups), the benchmark is run directly without the safety net. Overridable on the CMake command line, e.g.
# -DDUNE_GDT_BENCHMARK_TIMEOUT_SECONDS=120 for local runs.
set(DUNE_GDT_BENCHMARK_TIMEOUT_SECONDS
    600
    CACHE STRING "Per-benchmark wall-clock timeout in seconds for the run_benchmarks target")
find_program(DUNE_GDT_TIMEOUT_EXECUTABLE NAMES timeout)

# The prefix is the same for every benchmark, so build it once.
if(DUNE_GDT_TIMEOUT_EXECUTABLE)
  set(_timeout_prefix ${DUNE_GDT_TIMEOUT_EXECUTABLE} ${DUNE_GDT_BENCHMARK_TIMEOUT_SECONDS})
else()
  set(_timeout_prefix)
endif()

set(_run_commands COMMAND ${CMAKE_COMMAND} -E make_directory ${_benchmark_report_dir})
foreach(target ${_benchmark_targets})
  list(
    APPEND
    _run_commands
    COMMAND
    ${CMAKE_COMMAND}
    -E
    env
    "DUNE_GDT_BENCHMARK_OUTPUT_DIR=${_benchmark_report_dir}"
    ${_timeout_prefix}
    $<TARGET_FILE:${target}>)
endforeach()
add_custom_target(
  run_benchmarks
  ${_run_commands}
  DEPENDS benchmarks
  USES_TERMINAL
  COMMENT "Run all dune-gdt benchmarks and write nanobench JSON reports to ${_benchmark_report_dir}")
