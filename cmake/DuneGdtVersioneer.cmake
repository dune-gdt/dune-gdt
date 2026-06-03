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

# We use git to determine our version. To be usable in https://cmake.org/cmake/help/latest/command/project.html#options
# the version needs to be of the form <major>[.<minor>[.<patch>[.<tweak>]]], where major.minor.patch are determined from
# the closest previous git tag, and tweak the number of commits since then. Note that it is not possible to encode the
# information about a dirty working directory in the version.
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
