# ~~~
# This file is part of the dune-gdt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-gdt
# Copyright 2009-2021 dune-gdt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   René Fritze     (2021)
# ~~~

# cmake-lint: disable=C0103,W0106
macro(DUNE_XT_MODULE_VERSION_FROM_GIT)
  # We use git to determine our version. To be usable in
  # https://cmake.org/cmake/help/latest/command/project.html#options the version needs to be of the form
  # <major>[.<minor>[.<patch>[.<tweak>]]], where major.minor.patch are determined from the closest previous git tag, and
  # tweak the number of commits since then. Note that it is not possible to encode the information about a dirty working
  # directory in the version.
  find_program(GIT_EXECUTABLE git REQUIRED)

  # first determine the version from the closest previous tag
  execute_process(
    # only match tags that start with "20" followed by a digit
    COMMAND ${GIT_EXECUTABLE} describe --abbrev=0 --tags --match "20[0-9]*"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE_VERSION
    ERROR_VARIABLE GIT_DESCRIBE_ERROR
    RESULT_VARIABLE GIT_DESCRIBE_ERROR_CODE
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if(GIT_DESCRIBE_ERROR_CODE)
    message(WARNING "Could not determine the closest previous version as git tag, falling back to 0.0.0! "
                    "Ensure that there exists a git tag of the form:\n    vMAJOR.MINOR.PATCH\n"
                    "The original error was: ${GIT_DESCRIBE_ERROR}")
    set(PROJECT_VERSION "0.0.0")
  else()
    # second, determine the number of commits since that tag
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD ^${GIT_DESCRIBE_VERSION}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_COUNT_SINCE_TAG
      ERROR_VARIABLE GIT_COUNT_SINCE_TAG_ERROR
      RESULT_VARIABLE GIT_COUNT_SINCE_TAG_ERROR_CODE
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    if(GIT_COUNT_SINCE_TAG_ERROR_CODE)
      message(WARNING "Could not determine the number of commits since ${GIT_DESCRIBE_VERSION}, " "falling back to 0! "
                      "The original error was: ${GIT_COUNT_SINCE_TAG_ERROR}")
      set(GIT_COUNT_SINCE_TAG "0")
    endif()

    # append the number of commits since the tag in case they are not zero
    if(NOT ${GIT_COUNT_SINCE_TAG} STREQUAL "0")
      set(GIT_DESCRIBE_VERSION "${GIT_DESCRIBE_VERSION}.${GIT_COUNT_SINCE_TAG}")
    endif()

    set(PROJECT_VERSION "${GIT_DESCRIBE_VERSION}")

  endif()
  message(STATUS "Setting project version as determined from git history: ${PROJECT_VERSION}")

  # We will use DUNE_GDT_GIT_VERSION in other scripts further below
  set(DUNE_GDT_GIT_VERSION ${GIT_DESCRIBE_VERSION})

  # The `DUNE_GDT_VERSION*` variables constantly regenerate from a cmake target. Instead of trying to overwrite them,
  # create our own set `DUNE_GDT_GIT_VERSION*`
  extract_major_minor_version("${GIT_DESCRIBE_VERSION}" DUNE_VERSION)
  set(DUNE_GDT_GIT_VERSION_MAJOR "${DUNE_VERSION_MAJOR}")
  set(DUNE_GDT_GIT_VERSION_MINOR "${DUNE_VERSION_MINOR}")
  set(DUNE_GDT_GIT_VERSION_REVISION "${DUNE_VERSION_REVISION}")
  set(CPACK_PACKAGE_NAME "${DUNE_MOD_NAME}")
  set(CPACK_PACKAGE_VERSION "${DUNE_VERSION_MAJOR}.${DUNE_VERSION_MINOR}.${DUNE_VERSION_REVISION}")
  set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
  set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_BINARY_DIR}" "\\\\.svn" "\\\\.git" ".*/*\\\\.gitignore")

  # reset variables from dune-common/cmake/modules/DuneMacros.cmake:dune_project
  set(ProjectVersion "${GIT_DESCRIBE_VERSION}")
  set(ProjectVersionString "${DUNE_VERSION_MAJOR}.${DUNE_VERSION_MINOR}.${DUNE_VERSION_REVISION}")
  set(ProjectVersionMajor "${DUNE_VERSION_MAJOR}")
  set(ProjectVersionMinor "${DUNE_VERSION_MINOR}")
  set(ProjectVersionRevision "${DUNE_VERSION_REVISION}")

  # need to re-run template insertion
  if(NOT EXISTS ${PROJECT_SOURCE_DIR}/${ProjectName}-config-version.cmake.in)
    file(
      WRITE ${PROJECT_BINARY_DIR}/CMakeFiles/${ProjectName}-config-version.cmake.in
      "set(PACKAGE_VERSION \"${ProjectVersionString}\")

  if(\"\${PACKAGE_FIND_VERSION_MAJOR}\" EQUAL \"${ProjectVersionMajor}\" AND
       \"\${PACKAGE_FIND_VERSION_MINOR}\" EQUAL \"${ProjectVersionMinor}\")
    set (PACKAGE_VERSION_COMPATIBLE 1) # compatible with newer
    if (\"\${PACKAGE_FIND_VERSION}\" VERSION_EQUAL \"${ProjectVersionString}\")
      set(PACKAGE_VERSION_EXACT 1) #exact match for this version
    endif()
  endif()
  ")
    set(CONFIG_VERSION_FILE ${PROJECT_BINARY_DIR}/CMakeFiles/${ProjectName}-config-version.cmake.in)
  else()
    set(CONFIG_VERSION_FILE ${PROJECT_SOURCE_DIR}/${ProjectName}-config-version.cmake.in)
  endif()
  configure_file(${CONFIG_VERSION_FILE} ${PROJECT_BINARY_DIR}/${ProjectName}-config-version.cmake @ONLY)

endmacro(DUNE_XT_MODULE_VERSION_FROM_GIT)
