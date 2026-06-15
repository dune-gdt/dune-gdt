# ~~~
# This file is part of the dune-xt project:
#   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
# Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
# License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
#      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
#          with "runtime exception" (http://www.dune-project.org/license.html)
# Authors:
#   Felix Schindler (2014 - 2016)
#   René Fritze     (2012 - 2016, 2019)
#   Tobias Leibner  (2015 - 2016, 2019 - 2020)
# ~~~

enable_testing()

dependencycheck(${xtcommon})

# GoogleTest is provided by vcpkg (see vcpkg.json, find_package(GTest) in the top-level CMakeLists.txt). gtest_dune_xt
# bundles our test helpers (common.cxx) and exposes the GoogleTest headers to every test binary linking gtest_dune_xt.
dune_library_add_sources(gtest_dune_xt SOURCES common.cxx)
if(BUILD_SHARED_LIBS)
  # vcpkg builds GoogleTest as a static archive. With shared dune libraries, linking it PUBLIC would pull the archive
  # into both libgtest_dune_xt and every consuming test binary, so GoogleTest's vague-linkage globals (the
  # testing::UnitTest singleton, flag registrations) get registered for destruction twice -> "double free" at exit.
  # Instead pull the whole archive into libgtest_dune_xt (so all GoogleTest symbols are exported from the single shared
  # library) and link it PRIVATE so test binaries resolve those symbols from that one copy. Headers are still propagated.
  target_link_libraries(gtest_dune_xt PRIVATE "$<LINK_LIBRARY:WHOLE_ARCHIVE,GTest::gtest>")
  target_include_directories(gtest_dune_xt SYSTEM PUBLIC
                             $<TARGET_PROPERTY:GTest::gtest,INTERFACE_INCLUDE_DIRECTORIES>)
else()
  target_link_libraries(gtest_dune_xt PUBLIC GTest::gtest)
endif()

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/main.hxx DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/dune/xt/test/)

add_subdir_tests(common)
add_subdir_tests(grid)
add_subdir_tests(la)
add_subdir_tests(functions)
