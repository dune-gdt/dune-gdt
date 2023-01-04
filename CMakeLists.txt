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

# set up project
project("dune-gdt" CXX)

cmake_minimum_required(VERSION 3.13)

if("${TRAVIS}" EQUAL "1")
  add_compile_options(-Werror -Wno-error=unknown-pragmas -Wno-error=pedantic)
endif()

if(NOT
   (dune-common_DIR
    OR dune-common_ROOT
    OR "${CMAKE_PREFIX_PATH}" MATCHES ".*dune-common.*"))
  string(REPLACE ${CMAKE_PROJECT_NAME} dune-common dune-common_DIR ${PROJECT_BINARY_DIR})
endif()

# find dune-common and set the module path
find_package(dune-common REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake/modules)

find_program(RUN_IN_ENV_SCRIPT gdt-run-in-dune-env PATHS /usr/local/bin/ ${CMAKE_CURRENT_BINARY_DIR}
                                                         ${CMAKE_CURRENT_BINARY_DIR}/../dune-testtools/ REQUIRED)

execute_process(COMMAND ln -s ${RUN_IN_ENV_SCRIPT} ${CMAKE_CURRENT_BINARY_DIR}/gdt-run-in-dune-env)
set(DXT_TEST_TIMEOUT
    1000
    CACHE STRING "gtest timeout in seconds")

# include the dune macros
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

# dune-gdt cmake includes
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

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
include(DuneUtils)
make_dependent_modules_sys_included()

# do not change order here
add_subdirectory(python)
make_dependent_modules_sys_included()

# enable headercheck
add_definitions("-DENABLE_HEADERCHECK=1")
add_format(${CMAKE_CURRENT_SOURCE_DIR})
include(GridUtils)

add_header_listing()

add_subdirectory(dune)
add_subdirectory("cmake/modules")

add_tidy(${CMAKE_CURRENT_SOURCE_DIR})
add_pylicense()

finalize_dune_project(GENERATE_CONFIG_H_CMAKE)
# patch config.h to remove warnings about unused HAVE_GRIDTYPE
execute_process(
  COMMAND sed -i -e "s,^\\([ ]*\\)\#if HAVE_GRIDTYPE$,\\1\#if defined(HAVE_GRIDTYPE) \\&\\& HAVE_GRIDTYPE,g"
          ${CMAKE_CURRENT_BINARY_DIR}/config.h)
