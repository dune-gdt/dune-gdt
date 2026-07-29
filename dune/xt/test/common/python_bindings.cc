// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/python.hh: guarded_bind() runs a pybind11 registrar and swallows exactly those
// std::runtime_errors which pybind11 raises for types/functions that have already been registered, while letting
// every other error through.
//
// The registrar is a plain std::function, so the error messages pybind11 would produce can be reproduced here
// without actually going through pybind11 (and without needing a running interpreter).

// NOTE: this file must NOT be called python.cc. DuneXTTesting.cmake derives the test target name from the file's
// basename ("test_" + basename), and a target named test_python already exists: dune-common creates it in
// DunePythonCommonMacros.cmake, and cmake/modules/DuneXTTesting.cmake keeps it as an aggregate target for backwards
// compatibility. The duplicate name makes add_executable() -- and therefore the whole CMake configure step -- fail.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <stdexcept>
#include <string>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/python.hh>

using namespace Dune::XT::Common;


GTEST_TEST(guarded_bind, a_successful_registrar_is_simply_run)
{
  bool called = false;
  EXPECT_NO_THROW(bindings::guarded_bind([&]() { called = true; }));
  EXPECT_TRUE(called);
}


GTEST_TEST(guarded_bind, an_already_registered_type_is_tolerated)
{
  // This is the message pybind11 produces in detail::generic_type::initialize().
  EXPECT_NO_THROW(bindings::guarded_bind(
      []() { throw std::runtime_error("generic_type: type \"SomeType\" is already registered!"); }));
}


GTEST_TEST(guarded_bind, an_already_defined_function_is_tolerated)
{
  EXPECT_NO_THROW(
      bindings::guarded_bind([]() { throw std::runtime_error("function \"some_function\" is already defined"); }));
}


GTEST_TEST(guarded_bind, any_other_runtime_error_is_rethrown)
{
  EXPECT_THROW(bindings::guarded_bind([]() { throw std::runtime_error("something else went wrong"); }),
               std::runtime_error);
  // An empty message must not be mistaken for a duplicate registration either.
  EXPECT_THROW(bindings::guarded_bind([]() { throw std::runtime_error(""); }), std::runtime_error);
}


GTEST_TEST(guarded_bind, exceptions_which_are_no_runtime_errors_pass_through_unchanged)
{
  EXPECT_THROW(bindings::guarded_bind([]() { throw std::logic_error("not a runtime_error"); }), std::logic_error);
  EXPECT_THROW(bindings::guarded_bind([]() { DUNE_THROW(Exceptions::internal_error, "not a runtime_error"); }),
               Exceptions::internal_error);
}
