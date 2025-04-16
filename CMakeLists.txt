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

cmake_minimum_required(VERSION 3.29)
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

if("${TRAVIS}" EQUAL "1")
  add_compile_options(-Werror -Wno-error=unknown-pragmas -Wno-error=pedantic)
endif()

# allow dune to see imported targets across modules
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL ON)

# let 'find_package' know where modules can be found once configured removed dune-grid-glue: our gridview wrapper will
# need changes to work again. Seems Entity types on the glued view are different than in 2.7 removed dune-uggrid: lots
# of undefined refs
set(DUNE_SUBMODULES "dune-common;dune-geometry;dune-grid;dune-istl;dune-testtools;dune-localfunctions;dune-alugrid")
foreach(module IN LISTS DUNE_SUBMODULES)
  find_package(${module} CONFIG REQUIRED)
  list(APPEND CMAKE_MODULE_PATH ${${module}_MODULE_PATH})
endforeach()

# Dune::UGGrid
set(GRID_LIBS Dune::ALUGrid)

# find_program(RUN_IN_ENV_SCRIPT gdt-run-in-dune-env PATHS /usr/local/bin/ ${CMAKE_CURRENT_BINARY_DIR}
# ${CMAKE_CURRENT_BINARY_DIR}/../dune-testtools/ REQUIRED)

# execute_process(COMMAND ln -s ${RUN_IN_ENV_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/gdt-run-in-dune-env)
set(DXT_TEST_TIMEOUT
    1000
    CACHE STRING "gtest timeout in seconds")

# include the dune macros
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)
include(DuneMacros)
include(DuneUtils)

# start a dune project with information from dune.module
dune_project()

# can't use the run-in-env script here, because it will try to use dune.common which we're trying to install can't use
# dune-common_WHEELHOUSE here, because it is not set yet can't use a wildcard here, because no shell expansion is done
execute_process(
  COMMAND ${CMAKE_BINARY_DIR}/dune-env/bin/python -m pip install
          ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/share/dune/wheelhouse/dune_common-2.10.0-py3-none-any.whl
  COMMAND ${CMAKE_BINARY_DIR}/dune-env/bin/python -m pip install
          ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/share/dune/wheelhouse/dune_testtools-2.4-py3-none-any.whl
  OUTPUT_VARIABLE pip_install_log ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE
  ERROR_VARIABLE pip_install_log
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(DUNE_XT_WITH_PYTHON_BINDINGS)
  dune_enable_all_packages(INCLUDE_DIRS ${dune-gdt_SOURCE_DIR}/dune ${Python3_INCLUDE_DIRS} MODULE_LIBRARIES dunegdt)
else()
  dune_enable_all_packages(INCLUDE_DIRS ${dune-gdt_SOURCE_DIR}/dune ${Python3_INCLUDE_DIRS})
endif()

# (re-)set version info from git if available
include(DuneXtVersionHelper)
dune_xt_module_version_from_git()

find_package(Python3 COMPONENTS Interpreter Development)
add_subdirectory(deps/pybind11)
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

# do not change order here
add_subdirectory(python)
make_dependent_modules_sys_included()

# enable headercheck
add_definitions("-DENABLE_HEADERCHECK=1")
include(GridUtils)

add_header_listing()

add_subdirectory(dune)
add_subdirectory("cmake/modules")
add_subdirectory(examples)
add_subdirectory(tutorials)

add_tidy(${CMAKE_CURRENT_SOURCE_DIR})
add_pylicense()

finalize_dune_project(GENERATE_CONFIG_H_CMAKE)
# patch config.h to remove warnings about unused HAVE_GRIDTYPE
execute_process(
  COMMAND sed -i -e "s,^\\([ ]*\\)\#if HAVE_GRIDTYPE$,\\1\#if defined(HAVE_GRIDTYPE) \\&\\& HAVE_GRIDTYPE,g"
          ${CMAKE_CURRENT_BINARY_DIR}/config.h)
