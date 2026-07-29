// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   René Fritze    (2026)

// Covers dune/xt/common/numeric_cast.hh: the successful conversions as well as the error branch, in which the
// boost::bad_numeric_cast is translated into an external_error carrying the offending value and the target type.

#include <dune/xt/test/main.hxx> // <- This one has to come first, includes config.h!

#include <cstdint>
#include <limits>
#include <string>

#include <dune/xt/common/exceptions.hh>
#include <dune/xt/common/numeric_cast.hh>

using namespace Dune::XT::Common;


GTEST_TEST(numeric_cast, representable_values_are_converted)
{
  EXPECT_EQ(17, numeric_cast<int>(size_t(17)));
  EXPECT_EQ(size_t(17), numeric_cast<size_t>(17));
  EXPECT_EQ(int8_t(-1), numeric_cast<int8_t>(-1));
  EXPECT_EQ(3, numeric_cast<int>(3.0));
  EXPECT_DOUBLE_EQ(3., numeric_cast<double>(3));
  // Boundaries are still representable.
  EXPECT_EQ(std::numeric_limits<int>::max(), numeric_cast<int>(int64_t(std::numeric_limits<int>::max())));
  EXPECT_EQ(std::numeric_limits<int>::min(), numeric_cast<int>(int64_t(std::numeric_limits<int>::min())));
}


GTEST_TEST(numeric_cast, a_negative_source_does_not_fit_into_an_unsigned_target)
{
  EXPECT_THROW(numeric_cast<size_t>(-1), Exceptions::external_error);
  EXPECT_THROW(numeric_cast<unsigned int>(-1), Exceptions::external_error);
}


GTEST_TEST(numeric_cast, an_out_of_range_source_does_not_fit_into_a_narrower_target)
{
  EXPECT_THROW(numeric_cast<int8_t>(1000), Exceptions::external_error);
  EXPECT_THROW(numeric_cast<int>(int64_t(std::numeric_limits<int>::max()) + 1), Exceptions::external_error);
  EXPECT_THROW(numeric_cast<int>(int64_t(std::numeric_limits<int>::min()) - 1), Exceptions::external_error);
}


GTEST_TEST(numeric_cast, the_failure_message_names_the_value_and_the_target_type)
{
  try {
    numeric_cast<int8_t>(1000);
    FAIL() << "numeric_cast did not throw!";
  } catch (const Exceptions::external_error& ee) {
    const std::string what(ee.what());
    EXPECT_NE(std::string::npos, what.find("1000")) << what;
    EXPECT_NE(std::string::npos, what.find(Typename<int8_t>::value())) << what;
  }
}
