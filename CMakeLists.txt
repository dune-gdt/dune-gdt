# ~~~
# This file is part of the dune-gdt project:
# https://github.com/dune-community/dune-gdt
# Copyright 2010-2018 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
# or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
# with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
# Felix Schindler (2012 - 2018)
# René Fritze     (2014, 2016 - 2018)
# Sven Kaulmann   (2013)
# Tim Keil        (2018)
# Tobias Leibner  (2016, 2018 - 2021)
# ~~~

cmake_minimum_required(VERSION 3.29...4.0)
# To allow VERSION
cmake_policy(SET CMP0048 NEW)
# do not load custom FindBoost, use standard find_package mechanisms
cmake_policy(SET CMP0167 NEW)

# We automatically determine our PROJECT_VERSION from git, which needs to happen before the project() call.
include(cmake/DuneGdtVersioneer.cmake)
include(cmake/VcpkgBootStrap.cmake)

project(
  dune-gdt
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "Generic discretization toolbox for grid-based numerical methods."
  HOMEPAGE_URL "https://github.com/dune-gdt/dune-gdt/"
  LANGUAGES CXX)

# needs to come before anything else DUNE
find_package(dune-common REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake/modules")

# allow dune to see imported targets across modules
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

set(FEATURE_MODULES)
set(GRID_LIBS Dune::ALUGrid Dune::Grid Dune::ISTL Dune::Geometry)

if("uggrid" IN_LIST VCPKG_MANIFEST_FEATURES)
  list(APPEND FEATURE_MODULES "dune-uggrid")
  list(APPEND GRID_LIBS Dune::UGGrid)
endif()

if("alberta" IN_LIST VCPKG_MANIFEST_FEATURES)
  # alberta itself does not export its dependencies, so we need to find them manually
  find_library(TIRPC_LIB tirpc REQUIRED)
endif()

# let 'find_package' know where modules can be found once configured removed dune-grid-glue: our gridview wrapper will
# need changes to work again. Seems Entity types on the glued view are different than in 2.7 removed dune-uggrid: lots
# of undefined refs
set(DUNE_SUBMODULES "dune-common;dune-geometry;dune-grid;dune-istl;dune-testtools;dune-localfunctions;dune-alugrid")
foreach(module IN LISTS FEATURE_MODULES DUNE_SUBMODULES)
  find_package(${module} CONFIG REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${${module}_MODULE_PATH})
endforeach()

# Prefer the interpreter uv provides (pinned by the repo-root .python-version) as the single source of truth for "which
# Python". This guarantees the compiled bindings' ABI matches the interpreter the Python test suites run under (they
# also go through `uv run`, see DuneXTTesting.cmake). The hint set below is consumed by find_package(Python) immediately
# afterwards, by pybind11 (its vcpkg overlay is built with PYBIND11_NOPYTHON=ON, so it reuses these find_package(Python)
# results rather than searching on its own), and by dune-common's Python3 detection in include(DuneMacros) further down.
#
# DXT_USE_UV_PYTHON must be turned OFF for builds that have to use a specific pre-existing interpreter: the manylinux
# wheel build (.ci/build-wheels.sh) compiles against the container's CPython inside an already-activated venv into which
# dune-common installs the `dune` package; forcing a uv interpreter there desyncs dune-common's virtualenv handling from
# that active venv and breaks the configure-time system-test extraction (`dune_extract_static.py: No module named
# dune`).
option(DXT_USE_UV_PYTHON "Resolve the Python interpreter via uv (pinned by .python-version)" ON)
if(DXT_USE_UV_PYTHON)
  find_program(UV_EXECUTABLE uv)
endif()
if(UV_EXECUTABLE)
  # `uv python find` only locates an already-available interpreter; it does not download one. On a host/runner whose
  # system Python doesn't match the pinned (.python-version) interpreter, resolve it in two steps: try to find it, and
  # if that fails install it with `uv python install` (uv downloads a managed CPython) and find it again.
  execute_process(
    COMMAND ${UV_EXECUTABLE} python find
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE _uv_python_executable
    OUTPUT_STRIP_TRAILING_WHITESPACE
    RESULT_VARIABLE _uv_python_result)
  if(NOT _uv_python_result EQUAL 0)
    message(STATUS "uv has no matching interpreter yet; installing the pinned one via `uv python install`")
    execute_process(
      COMMAND ${UV_EXECUTABLE} python install
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      RESULT_VARIABLE _uv_install_result)
    if(NOT _uv_install_result EQUAL 0)
      message(
        FATAL_ERROR "`uv python install` failed (exit ${_uv_install_result}); cannot provide the Python interpreter")
    endif()
    execute_process(
      COMMAND ${UV_EXECUTABLE} python find
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE _uv_python_executable
      OUTPUT_STRIP_TRAILING_WHITESPACE
      RESULT_VARIABLE _uv_python_result)
  endif()
  if(NOT _uv_python_result EQUAL 0)
    message(FATAL_ERROR "`uv python find` failed (exit ${_uv_python_result}); cannot resolve the Python interpreter")
  endif()
  # Python_EXECUTABLE / Python3_EXECUTABLE are the CMake-mandated FindPython hint names, so the C0103 cache-name
  # convention does not apply to them.

  # cmake-lint: disable=C0103
  set(Python_EXECUTABLE
      "${_uv_python_executable}"
      CACHE FILEPATH "Python interpreter provided by uv" FORCE)
  # cmake-lint: disable=C0103
  set(Python3_EXECUTABLE
      "${_uv_python_executable}"
      CACHE FILEPATH "Python interpreter provided by uv" FORCE)
  message(STATUS "Using uv-provided Python interpreter: ${_uv_python_executable}")
else()
  message(STATUS "uv disabled or unavailable (DXT_USE_UV_PYTHON=${DXT_USE_UV_PYTHON}); using default FindPython")
endif()

find_package(Python COMPONENTS Interpreter Development)
find_package(pybind11 CONFIG REQUIRED)

# GoogleTest is provided by vcpkg (see vcpkg.json) and used by the gtest_dune_xt test-helper library.
find_package(GTest CONFIG REQUIRED)

# find_program(RUN_IN_ENV_SCRIPT gdt-run-in-dune-env PATHS /usr/local/bin/ ${CMAKE_CURRENT_BINARY_DIR}
# ${CMAKE_CURRENT_BINARY_DIR}/../dune-testtools/ REQUIRED) execute_process(COMMAND ln -s ${RUN_IN_ENV_SCRIPT}
# ${CMAKE_CURRENT_BINARY_DIR}/gdt-run-in-dune-env)

# include the dune macros
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(DuneMacros)
include(DuneUtils)

# start a dune project with information from dune.module
dune_project()

if(DUNE_XT_WITH_PYTHON_BINDINGS)
  dune_enable_all_packages(INCLUDE_DIRS ${dune-gdt_SOURCE_DIR}/dune ${Python3_INCLUDE_DIRS} MODULE_LIBRARIES dunegdt)
else()
  dune_enable_all_packages(INCLUDE_DIRS ${dune-gdt_SOURCE_DIR}/dune ${Python3_INCLUDE_DIRS})
endif()

# (re-)set version info from git if available
include(DuneXtVersionHelper)
dune_xt_module_version_from_git()

dune_enable_all_packages(
  INCLUDE_DIRS
  ${dune-gdt_SOURCE_DIR}/dune
  ${Python3_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/pybind11/include
  MODULE_LIBRARIES
  gtest_dune_xt
  dunext)

if(NOT DXT_DONT_LINK_PYTHON_LIB)
  target_link_libraries(dunext INTERFACE ${Python3_LIBRARIES})
endif()
target_include_directories(dunext SYSTEM INTERFACE $<BUILD_INTERFACE:${pybind11_INCLUDE_DIR}>
                                                   $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

# target_include_directories( dunext SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/deps/pybind11/include)
target_compile_features(dunext INTERFACE cxx_inheriting_constructors cxx_user_literals cxx_right_angle_brackets)
# ----------------------------------------------------------------------------------------------------
# General Settings
# ----------------------------------------------------------------------------------------------------

# search for spe10 data files
find_file(
  SPE10MODEL1DATA
  NAMES perm_case1.dat
  PATHS "${dune-gdt_SOURCE_DIR}/../local/src/spe10/model1"
  DOC "Location of perm_case1.dat"
  NO_DEFAULT_PATH)

# silence most warnings we are not responsible for
make_dependent_modules_sys_included()

# GCC 12 added -Wdangling-reference (enabled by -Wall) but incorrectly fires on QuadratureRules::rule(), which returns a
# static cache reference (GCC bug #106372). Suppress this false positive globally for GCC >= 12.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12")
  add_compile_options(-Wno-dangling-reference)
endif()

# do not change order here
add_subdirectory(python)
make_dependent_modules_sys_included()

# TODO: this isn't actually the way to do it??? enable headercheck add_definitions("-DENABLE_HEADERCHECK=1")
include(GridUtils)

add_header_listing()

add_subdirectory(dune)
add_subdirectory("cmake/modules")
add_subdirectory(examples)
add_subdirectory(benchmarks)
add_subdirectory(docs)

add_tidy(${CMAKE_CURRENT_SOURCE_DIR})
add_pylicense()

dxt_exclude_from_headercheck(${CMAKE_SOURCE_DIR}/.vcpkg-root)
dxt_exclude_from_headercheck(${CMAKE_BINARY_DIR}/vcpkg_installed)

finalize_dune_project()
# patch config.h to remove warnings about unused HAVE_GRIDTYPE
execute_process(
  COMMAND sed -i -e "s,^\\([ ]*\\)\#if HAVE_GRIDTYPE$,\\1\#if defined(HAVE_GRIDTYPE) \\&\\& HAVE_GRIDTYPE,g"
          ${CMAKE_CURRENT_BINARY_DIR}/config.h)
