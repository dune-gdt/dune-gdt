// This file is part of the dune-xt project:
//   https://zivgitlab.uni-muenster.de/ag-ohlberger/dune-community/dune-xt
// Copyright 2009-2021 dune-xt developers and contributors. All rights reserved.
// License: Dual licensed as BSD 2-Clause License (http://opensource.org/licenses/BSD-2-Clause)
//      or  GPL-2.0+ (http://opensource.org/licenses/gpl-license)
//          with "runtime exception" (http://www.dune-project.org/license.html)
// Authors:
//   Felix Schindler (2012, 2014, 2016 - 2017)
//   René Fritze     (2012 - 2013, 2015 - 2016, 2018 - 2019)
//   Tobias Leibner  (2014, 2016, 2018 - 2020)

#include <dune/xt/test/main.hxx>

#include <cstring>
#include <string>

#include <dune/xt/common/debug.hh>

using namespace Dune::XT::Common;

GTEST_TEST(debug, main)
{
#ifndef NDEBUG
  EXPECT_THROW(DXT_ASSERT(false), Dune::XT::Common::Exceptions::debug_assertion);
#else
  EXPECT_NO_THROW(DXT_ASSERT(false));
#endif
}

GTEST_TEST(debug, a_true_assertion_never_throws)
{
  EXPECT_NO_THROW(DXT_ASSERT(true));
  EXPECT_NO_THROW(DXT_ASSERT(1 + 1 == 2));
}

GTEST_TEST(debug, the_assertion_message_names_the_condition)
{
#ifndef NDEBUG
  try {
    const int forty_two = 42;
    DXT_ASSERT(forty_two < 0);
    FAIL() << "DXT_ASSERT did not throw!";
  } catch (const Dune::XT::Common::Exceptions::debug_assertion& ee) {
    const std::string what(ee.what());
    EXPECT_NE(std::string::npos, what.find("Assertion failed")) << what;
    EXPECT_NE(std::string::npos, what.find("forty_two < 0")) << what;
  }
#endif
}

GTEST_TEST(debug, charcopy)
{
  const char* original = "some string";
  char* copy = charcopy(original);
  ASSERT_NE(nullptr, copy);
  // A copy, not an alias ...
  EXPECT_NE(original, copy);
  // ... which is null terminated and holds the same characters.
  EXPECT_STREQ(original, copy);
  EXPECT_EQ('\0', copy[std::strlen(original)]);
  delete[] copy;

  char* empty = charcopy("");
  ASSERT_NE(nullptr, empty);
  EXPECT_EQ('\0', empty[0]);
  delete[] empty;
}

GTEST_TEST(debug, DXTC_DEBUG_AUTO)
{
  // The macro merely declares a (maybe unused) volatile variable, so that it survives the optimizer.
  DXTC_DEBUG_AUTO(value) = 42;
  EXPECT_EQ(42, value);
}
