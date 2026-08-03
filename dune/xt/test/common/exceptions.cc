// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/exceptions.hh/.cc: the exception hierarchy, DUNE_THROW_IF and the two handle_exception()
// overloads which are meant to be called from a catch-all in main().
//
// handle_exception() dumps the timings and the memory usage and then calls abort_all_mpi_processes(), which is a
// no-op returning 1 as long as there is a single rank -- which is the case here, since this test is not run with
// several MPI ranks (see the "mpi" filename convention in cmake/modules/DuneXTTesting.cmake).

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <stdexcept>
#include <string>

#include <boost/filesystem.hpp>

#include <dune/common/exceptions.hh>

#include <dune/xt/common/exceptions.hh>

using namespace Dune::XT::Common;


GTEST_TEST(exceptions, the_hierarchy_is_as_documented)
{
  // Everything derives from Dune::Exception, so a catch-all in main() catches all of them.
  EXPECT_THROW(DUNE_THROW(Exceptions::shapes_do_not_match, "msg"), Dune::Exception);
  EXPECT_THROW(DUNE_THROW(Exceptions::index_out_of_range, "msg"), Dune::Exception);
  EXPECT_THROW(DUNE_THROW(Exceptions::conversion_error, "msg"), Dune::Exception);
  EXPECT_THROW(DUNE_THROW(Exceptions::external_error, "msg"), Dune::Exception);
  EXPECT_THROW(DUNE_THROW(Exceptions::dependency_missing, "msg"), Dune::Exception);
  EXPECT_THROW(DUNE_THROW(Exceptions::logger_error, "msg"), Dune::Exception);

  // The refinements of you_are_using_this_wrong have to be catchable through their base ...
  EXPECT_THROW(DUNE_THROW(Exceptions::wrong_input_given, "msg"), Exceptions::you_are_using_this_wrong);
  EXPECT_THROW(DUNE_THROW(Exceptions::requirements_not_met, "msg"), Exceptions::you_are_using_this_wrong);
  EXPECT_THROW(DUNE_THROW(Exceptions::bisection_error, "msg"), Exceptions::wrong_input_given);
  // ... and the same holds for the two which extend dune-common's exceptions.
  EXPECT_THROW(DUNE_THROW(Exceptions::you_have_to_implement_this, "msg"), Dune::NotImplemented);
  EXPECT_THROW(DUNE_THROW(Exceptions::this_should_not_happen, "msg"), Dune::InvalidStateException);
}


GTEST_TEST(exceptions, the_message_is_preserved)
{
  try {
    DUNE_THROW(Exceptions::configuration_error, "a very specific detail");
    FAIL() << "DUNE_THROW did not throw!";
  } catch (const Exceptions::configuration_error& ee) {
    EXPECT_NE(std::string::npos, std::string(ee.what()).find("a very specific detail")) << ee.what();
  }
}


GTEST_TEST(exceptions, DUNE_THROW_IF)
{
  EXPECT_NO_THROW(DUNE_THROW_IF(false, Exceptions::internal_error, "not thrown"));
  EXPECT_THROW(DUNE_THROW_IF(true, Exceptions::internal_error, "thrown"), Exceptions::internal_error);
}


GTEST_TEST(exceptions, handle_exception_for_a_dune_exception)
{
  try {
    DUNE_THROW(Exceptions::internal_error, "handled by handle_exception");
  } catch (const Dune::Exception& ee) {
    // With a single rank abort_all_mpi_processes() does not abort anything and just returns 1.
    EXPECT_EQ(1, handle_exception(ee));
    return;
  }
  FAIL() << "DUNE_THROW did not throw!";
}


GTEST_TEST(exceptions, handle_exception_for_a_std_exception)
{
  const std::runtime_error exception("handled by handle_exception");
  EXPECT_EQ(1, handle_exception(exception));
}
